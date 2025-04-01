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


class ConsensusModule final : public raft::Consensus::CallbackService, public std::enable_shared_from_this<ConsensusModule> {
public:

    ConsensusModule(const ClusterConfig& config);

    bool ConnectToPeers();

    void StartElectionTimer();

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

    using ClientCtxPtr = std::unique_ptr<grpc::ClientContext>;
    using AtomicIntPtr = std::shared_ptr<std::atomic<int>>;
    using AtomicBoolPtr = std::shared_ptr<std::atomic<bool>>;
    using StubPtr = std::unique_ptr<raft::Consensus::Stub>;

    using milliseconds = std::chrono::milliseconds;

    void OnElectionTimeout();

    void SendLeaderHeartbeat();

    void StartLeaderHeartbeat();

    void BroadcastHeartbeat();

    ClusterConfig config_;
    std::atomic<State> state_;
    milliseconds election_timeout_;
    std::shared_ptr<grpc_raft::Timer> election_timer_;

    // Book keeping when requesting votes.
    AtomicIntPtr election_count_ = nullptr;
    AtomicBoolPtr new_leader_discovered_ = nullptr;

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

    /// Hold onto a vote so that it doesn't go out of scope.
    raft::VoteRequest vote_request_;

    /// Ditto if we're the leader
    raft::AppendEntriesRequest append_entries_request_;

    /// Persistent votes, otherwise the pointer for the
    /// async call is dangling.
    std::vector<raft::VoteResponse> votes_;

    /// Persistent for when we're the leader
    std::vector<raft::AppendEntriesResponse> append_entries_responses_;

    /// Contexts for getting responses;
    std::vector<ClientCtxPtr> contexts_;

    ///
    int leader_rank_ = -1;

    /// Broadcast timer
    std::shared_ptr<grpc_raft::Timer> leader_timer_;

    /// Leader heartbeat, just needs to be << timeout
    milliseconds leader_timeout_;
};
