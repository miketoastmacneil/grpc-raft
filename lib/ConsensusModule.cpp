//
// Created by Mike MacNeil on 3/17/25.
//

#include "ConsensusModule.h"

#include <cstdint>
#include <absl/random/random.h>
#include <grpcpp/create_channel.h>

ConsensusModule::ConsensusModule(const ClusterConfig& config):config_{config} {
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

ServerUnaryReactor *ConsensusModule::SetValue(grpc::CallbackServerContext *context, const raft::LogEntry *entry, raft::LogEntryResponse *response) {
    ServerUnaryReactor* reactor = context->DefaultReactor();
    reactor->Finish(grpc::Status::OK);
    return reactor;
}

ServerUnaryReactor *ConsensusModule::GetValue(grpc::CallbackServerContext *context, const raft::GetRequest *request_key, raft::GetResponse *response) {
    ServerUnaryReactor* reactor = context->DefaultReactor();
    reactor->Finish(grpc::Status::OK);
    return reactor;
}

ServerUnaryReactor* ConsensusModule::AppendEntries(grpc::CallbackServerContext * context, const raft::AppendEntriesRequest * request, raft::AppendEntriesResponse * response) {
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
        std::cout << "Election timeout for rank: "<< config_.rank() << std::endl;
        // Vote for myself then send out the request RPC
        state_ = State::CANDIDATE;
        election_count_ = std::make_shared<std::atomic<int>>(1);
        auto num_others = config_.other_ports().size();
    }

    if (state_ == State::CANDIDATE) {
        //
    }
}
