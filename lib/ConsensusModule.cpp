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

    state_ = State::FOLLOWER;
    grpc_raft::Timer raft_timer(election_timeout_, [this]() {
        OnElectionTimeout();
    });
    election_timer_ = std::move(raft_timer);

}

bool ConsensusModule::ConnectToPeers() {
    int connected = 0;
    for (auto value:config_.other_ports()) {
        auto channel = grpc::CreateChannel(value, grpc::InsecureChannelCredentials());
        if (channel->WaitForConnected(std::chrono::system_clock::now() + std::chrono::milliseconds(1000))) {
            std::cout << "Connected to " << value << "\n";
            connected++;
        }
        auto stub = raft::Consensus::NewStub(channel);
        cluster_stubs_.push_back(std::move(stub));
    }
    if (connected == config_.other_ports().size()) {
        return true;
    } else {
        return false;
    }
}

void ConsensusModule::Start() {
    election_timer_.start();
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
    reactor->Finish(grpc::Status::OK);
    return reactor;
}

ServerUnaryReactor* ConsensusModule::RequestVote(grpc::CallbackServerContext * context, const raft::VoteRequest * request, raft::VoteResponse * response) {
    ServerUnaryReactor* reactor = context->DefaultReactor();
    reactor->Finish(grpc::Status::OK);
    return reactor;
}
void ConsensusModule::OnElectionTimeout() {
    if (state_ == State::FOLLOWER) {
        LOG(INFO) << "Election timeout for rank: " << config_.rank() << std::endl;
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
        auto cluster_count_excluding_myself = config_.other_ports().size();

        votes_.clear();
        votes_.resize(cluster_count_excluding_myself);
        contexts_.clear();
        for (int i = 0; i < cluster_count_excluding_myself; ++i) {
            auto ctx = std::make_unique<grpc::ClientContext>();
            contexts_.push_back(std::move(ctx));
        }

        for (int i = 0; i < cluster_count_excluding_myself; i++) {
            // Don't want responses to outlive this call.
            cluster_stubs_[i]->async()->RequestVote(contexts_[i].get(), &vote_request_, &votes_[i], [this, i](grpc::Status status) {
                if (status.ok()) {
                    LOG(INFO) << "Vote request successfully completed.";
                    if (votes_[i].votegranted()) {
                        *election_count_ += 1;
                        if (*election_count_ >= 2) {
                            TransitionToLeader();
                        }
                    }
                }
            });
        }
    }

    if (state_ == State::CANDIDATE) {

    }
}

void ConsensusModule::TransitionToLeader() {

}

