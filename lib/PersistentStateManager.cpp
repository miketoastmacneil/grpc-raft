//
// Created by Mike MacNeil on 3/31/25.
//

#include "PersistentStateManager.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <shared_mutex>

namespace fs = std::filesystem;

PersistentState PersistentState::FromJson(const json &j) {
    PersistentState state;
    state.current_term = j["current_term"].get<int>();
    state.voted_for = j["voted_for"].get<int>();
    return state;
}


json PersistentState::ToJson() const {
    return {{"current_term", current_term}, {"voted_for", voted_for}};
}

PersistentStateManager::PersistentStateManager(const std::string& path)
    : root_path_(path) {}


void PersistentStateManager::AppendLogEntry(const raft::LogEntry& entry) {
    std::ofstream ofs(root_path_, std::ios::app);
    if (!ofs) throw std::runtime_error("Failed to open log file");
    ofs << entry.SerializeAsString() << "\n";
    ofs.flush();
}

std::vector<raft::LogEntry> PersistentStateManager::LogEntries() const {
    std::ifstream ifs(root_path_);
    std::vector<raft::LogEntry> entries;
    std::string line;

    while (std::getline(ifs, line)) {
        if (line.empty()) continue;

        raft::LogEntry entry;
        if (entry.ParseFromString(line)) {
            entries.push_back(entry);
        }
    }

    return entries;
}

void PersistentStateManager::SetState(const PersistentState &state) {
    fs::path state_path = fs::path(root_path_) / state_path_stem_;
    auto output = state.ToJson();
    std::ofstream ofs(state_path);
    ofs << state.ToJson().dump(4);
    ofs.flush();
    ofs.close();
}


PersistentState PersistentStateManager::GetState() const {
    fs::path state_path = fs::path(root_path_) / state_path_stem_;
    if (!fs::exists(state_path)) {
        if (!fs::create_directories(state_path.parent_path())) {
            throw std::runtime_error("Failed to create directory " + state_path.root_directory().string());
        }
        // Create the first empty state, persist and return
        PersistentState first_state{.current_term = 0, .voted_for = -1};
        std::ofstream ofs(state_path);
        if (!ofs) throw std::runtime_error(std::format("Failed to open state file {}", state_path.string()));
        ofs << first_state.ToJson().dump(4);
        ofs.flush();
        ofs.close();
        return first_state;
    } else {
        std::ifstream ifs(state_path);
        json j;
        ifs >> j;
        PersistentState result = PersistentState::FromJson(j);
        ifs.close();
        return result;
    }
}