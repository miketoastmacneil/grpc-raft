//
// Created by Mike MacNeil on 3/17/25.
//

#include "ConsensusModule.h"

#include <cstdint>
#include <absl/log/log.h>
#include <absl/log/initialize.h>
#include <absl/random/random.h>
#include <grpcpp/create_channel.h>

ConsensusModule::ConsensusModule(const ClusterConfig& config):config_{config} {

    // Logging from std out, not the log we're maintaining
    absl::InitializeLog();

    // Any logic around what we need from the log can be handled here.
    log_manager_ = PersistentStateManager(config.log_path());

    // Timeout is created randomly even when recovering from a crash.
    absl::BitGen bitgen;
    uint32_t ms_time = absl::uniform_int_distribution<uint32_t>(config.timeout_min(), config.timeout_max())(bitgen);
    election_timeout_ = milliseconds(ms_time);
    leader_timeout_ = milliseconds(500);

    state_ = State::FOLLOWER;
    election_timer_ = std::make_unique<grpc_raft::Timer>();
    leader_timer_ = std::make_unique<grpc_raft::Timer>();
}

bool ConsensusModule::ConnectToPeers() {
    int connected = 0;
    for (auto value:config_.other_ports()) {
        auto channel = grpc::CreateChannel(value, grpc::InsecureChannelCredentials());
        if (channel->WaitForConnected(std::chrono::system_clock::now() + std::chrono::milliseconds(1000))) {
            std::cout << "Connected to " << value << "\n";
            connected++;
            auto stub = raft::Consensus::NewStub(channel);
            cluster_stubs_.push_back(std::move(stub));
        }
    }
    if (connected == config_.other_ports().size()) {
        return true;
    } else {
        return false;
    }
}

void ConsensusModule::StartElectionTimer() {
    auto self = shared_from_this();
    election_timer_->Start(election_timeout_, [self]() {
        self->OnElectionTimeout();
    });
}

void ConsensusModule::StartLeaderHeartbeat() {
    auto self = shared_from_this();
    leader_timer_->Start(leader_timeout_, [self]() {
        self->SendLeaderHeartbeat();
        self->StartLeaderHeartbeat();
    });
}

void ConsensusModule::BroadcastHeartbeat() {

}


ServerUnaryReactor *ConsensusModule::SetValue(grpc::CallbackServerContext* context, const raft::SetRequest* entry, raft::SetResponse* response) {
    ServerUnaryReactor* reactor = context->DefaultReactor();
    reactor->Finish(grpc::Status::OK);
    return reactor;
}

ServerUnaryReactor *ConsensusModule::GetValue(grpc::CallbackServerContext* context, const raft::GetRequest* request_key, raft::GetResponse* response) {
    ServerUnaryReactor* reactor = context->DefaultReactor();
    reactor->Finish(grpc::Status::OK);
    return reactor;
}

ServerUnaryReactor* ConsensusModule::AppendEntries(grpc::CallbackServerContext* context, const raft::AppendEntriesRequest* request, raft::AppendEntriesResponse* response) {
    ServerUnaryReactor* reactor = context->DefaultReactor();

    if (state_ == State::FOLLOWER) {
        if (request->logentries_size() == 0) {
            std::cout << "Leader heartbeat received" << request->leaderid() << "\n";
            leader_rank_ = request->leaderid();
            StartElectionTimer();
        }
    }

    if (state_ == State::CANDIDATE) {
        // Getting an empty request from new leader,
        if (request->logentries_size()) {
            leader_rank_ = request->leaderid();
            std::cout << "New leader found" << request->leaderid() << "\n";
            // Updated in case we're
            if (!new_leader_discovered_) {
                new_leader_discovered_ = std::make_shared<std::atomic<bool>>(true);
                StartElectionTimer();
            }
            state_ == State::FOLLOWER;
        }
    }
    reactor->Finish(grpc::Status::OK);
    return reactor;
}

ServerUnaryReactor* ConsensusModule::RequestVote(grpc::CallbackServerContext * context, const raft::VoteRequest * request, raft::VoteResponse * response) {
    ServerUnaryReactor* reactor = context->DefaultReactor();

    auto state = log_manager_.GetState();
    // If we're a candidate, we've already voted for ourselves.
    std::cout << "VoteRequest received from: " << request->candidateid() << "\n";
    if (state_ == State::CANDIDATE) {
        response->set_term(state.current_term);
        response->set_votegranted(false);
        reactor->Finish(grpc::Status::OK);
        return reactor;
    }

    if (request->term() == state.current_term) {
        response->set_term(state.current_term);
        response->set_votegranted(false);
    }

    if (request->term() > state.current_term) {
        state.current_term = request->term();
        // We haven't voted for anyone in this term so set to
        // null
        state.voted_for = -1;
    }

    if ((state.voted_for < 0) || (state.voted_for == request->candidateid())) {
        response->set_term(state.current_term);
        response->set_votegranted(true);
        std::cout << "Voting for " << request->candidateid() << "\n";
        state.voted_for = request->candidateid();
        log_manager_.SetState(state);
    }

    reactor->Finish(grpc::Status::OK);
    return reactor;
}
void ConsensusModule::OnElectionTimeout() {
    if (state_ == State::FOLLOWER) {
        std::cout << "Election timeout " << std::endl;
        state_ = State::CANDIDATE;
        vote_request_ = raft::VoteRequest();
        vote_request_.set_candidateid(config_.rank());
        auto log_entries = log_manager_.LogEntries();
        if (log_entries.empty()) {
            vote_request_.set_lastlogindex(0);
            vote_request_.set_lastlogterm(0);
        }
        auto state = log_manager_.GetState();
        PersistentState new_state = state;
        new_state.current_term += 1;
        // Increment term now that we're a candidate
        vote_request_.set_term(new_state.current_term);
        log_manager_.SetState(new_state);
        election_count_ = std::make_shared<std::atomic<int>>(1);
        new_leader_discovered_ = std::make_shared<std::atomic<bool>>(false);

        auto connected_peers_size = cluster_stubs_.size();

        votes_.clear();
        votes_.resize(connected_peers_size);
        contexts_.clear();
        for (int i = 0; i < connected_peers_size; ++i) {
            auto ctx = std::make_unique<grpc::ClientContext>();
            contexts_.push_back(std::move(ctx));
        }

        for (int i = 0; i < connected_peers_size; i++) {
            // Don't want responses to outlive this call.
            cluster_stubs_[i]->async()->RequestVote(contexts_[i].get(), &vote_request_, &votes_[i], [this, i](grpc::Status status) {
                if (status.ok()) {
                    LOG(INFO) << "Vote request successfully completed.";
                    if (new_leader_discovered_) {
                        if (*new_leader_discovered_) {
                            std::cout << "New leader discovered, cancelling election" << std::endl;
                            votes_.clear();
                            election_count_ = nullptr;
                            return;
                        }
                    }
                    if (votes_[i].votegranted()) {
                        *election_count_ += 1;
                        // Once we have a majority start
                        if (*election_count_ == 3) {
                            StartLeaderHeartbeat();
                        }
                    }
                }
            });
        }

    }

    if (state_ == State::CANDIDATE) {

    }
}

void ConsensusModule::SendLeaderHeartbeat() {
    std::cout << "Sending Leader Heartbeat" << std::endl;
    auto cluster_count_excluding_myself = config_.other_ports().size();
    contexts_.clear();
    for (int i = 0; i < cluster_count_excluding_myself; ++i) {
        auto ctx = std::make_unique<grpc::ClientContext>();
        contexts_.push_back(std::move(ctx));
    }

    raft::AppendEntriesRequest append_entries_request;
    append_entries_request.set_leaderid(config_.rank());
    append_entries_request.clear_logentries();
    append_entries_request_ = append_entries_request;
    append_entries_responses_.clear();
    append_entries_responses_.resize(cluster_count_excluding_myself);

    for (int i = 0; i < cluster_count_excluding_myself; i++) {
        // Don't want responses to outlive this call.
        cluster_stubs_[i]->async()->AppendEntries(contexts_[i].get(), &append_entries_request_, &append_entries_responses_[i], [this, i](grpc::Status status) {
            if (status.ok()) {
                std::cout << "Append entries successfully completed." << std::endl;
            }
        });
    }
}

