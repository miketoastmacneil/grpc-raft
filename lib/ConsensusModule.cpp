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

    for (auto value:config.other_ports()) {
        auto channel = grpc::CreateChannel(value, grpc::InsecureChannelCredentials());
        auto stub = raft::Consensus::NewStub(channel);
        cluster_stubs_.push_back(std::move(stub));
    }
    std::cout << "Server started: [rank: " << config.rank() << " listening on: " << config.port() << "]" <<std::endl;
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
        raft::VoteRequest request;
        request.set_candidateid(config_.rank());
        auto log_entries = log_manager_.LogEntries();
        if (log_entries.empty()) {
            request.set_lastlogindex(0);
            request.set_lastlogterm(0);
        }
        auto state = log_manager_.GetState();
        PersistentState new_state = state;
        new_state.current_term += 1;
        // Increment term now that we're a candidate
        request.set_term(new_state.current_term);
        log_manager_.SetState(new_state);
        election_count_ = std::make_shared<std::atomic<int>>(1);
        auto cluster_count_excluding_myself = config_.other_ports().size();

        votes_.clear();
        votes_.resize(cluster_count_excluding_myself);

        // for (int i = 0; i < cluster_count_excluding_myself; i++) {
        //     // Don't want responses to outlive this call.
        //     cluster_stubs_[i]->async()->RequestVote()
        // }
    }

    if (state_ == State::CANDIDATE) {

    }
}
