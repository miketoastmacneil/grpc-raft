//
// Created by Mike MacNeil on 3/31/25.
//

#pragma once

#include <string>
#include <vector>
#include "proto/Messages.pb.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct PersistentState {
    // Deserialized from json
    static PersistentState FromJson(const json& j);
    // Serialize to json
    json ToJson() const;

    // Latest term this server has seen
    int current_term;
    // Candidate id that received vote in current term.
    // If uninitialized, equal to -1.
    int voted_for;
};

class PersistentStateManager {
public:
    // Empty Default constructor
    PersistentStateManager() = default;

    // Constructs a State Manager with a possibly non-empty
    // state directory.
    explicit PersistentStateManager(const std::string& state_dir);

    // Append an entry to the log.
    void AppendLogEntry(const raft::LogEntry& entry);

    // Get all log entries
    std::vector<raft::LogEntry> LogEntries() const;

    // Get current state from stable storage
    PersistentState GetState() const;

    // Update the current state on stable storage
    void SetState(const PersistentState& state);

private:
    std::string root_path_;
    std::string state_path_stem_ = "state.json";
    std::string log_path_stem_ = "recorded_log.log";
};




