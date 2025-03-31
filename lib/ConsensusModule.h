//
// Created by Mike MacNeil on 3/17/25.
//


#pragma once

#include "ClusterConfig.h"
#include "PersistentStateManager.h"
#include "Timer.h"
#include "proto/Messages.pb.h"
#include "proto/Messages.grpc.pb.h"

using grpc::CallbackServerContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerUnaryReactor;

using AtomicIntPtr = std::shared_ptr<std::atomic<int>>;
using StubPtr = std::unique_ptr<raft::Consensus::Stub>;

class ConsensusModule final : public raft::Consensus::CallbackService {
public:

    ConsensusModule(const ClusterConfig& config);

    ServerUnaryReactor* SetValue(grpc::CallbackServerContext * context, const raft::SetRequest* entry, raft::SetResponse* response) override;

    ServerUnaryReactor* GetValue(grpc::CallbackServerContext * context, const raft::GetRequest * request_key, raft::GetResponse * response) override;

    ServerUnaryReactor* AppendEntries(grpc::CallbackServerContext * context, const raft::AppendEntriesRequest * request, raft::AppendEntriesResponse * response) override;

    ServerUnaryReactor* RequestVote(grpc::CallbackServerContext * context, const raft::VoteRequest * request, raft::VoteResponse * response) override;

private:

    enum class State {
        FOLLOWER = 0,
        CANDIDATE = 1,
        LEADER = 2,
    };

    using milliseconds = std::chrono::milliseconds;

    void OnElectionTimeout();

    ClusterConfig config_;
    std::atomic<State> state_;
    milliseconds election_timeout_;
    grpc_raft::Timer election_timer_;

    // Book keeping when requesting votes.
    AtomicIntPtr election_count_ = nullptr;
    std::vector<StubPtr> cluster_stubs_;

    /// index of highest known log entry to be
    /// committed
    std::atomic<int> commit_index_ = 0;

    /// Index of the highest log entry which has
    /// been applied to the state machine
    std::atomic<int> last_applied_index_ = 0;

    /// Manages the replicated log, source of truth
    /// for arguments for RPC requests.
    PersistentStateManager log_manager_;

    /// Persistent votes, otherwise the pointer for the
    /// async call is dangling.
    std::vector<raft::VoteResponse> votes_;
};
