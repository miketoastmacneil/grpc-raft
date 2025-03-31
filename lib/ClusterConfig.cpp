//
// Created by Mike MacNeil on 3/17/25.
//

#include <absl/strings/str_format.h>

#include "ClusterConfig.h"

#include <toml++/toml.hpp>

ClusterConfig ClusterConfig::FromToml(uint16_t rank, const fs::path& path) {
    toml::table parsed = toml::parse_file(path.string());
    auto ports = parsed["ports"].as_array();
    std::map<int, std::string> port_map;
    for (auto i = 0; i < ports->size(); ++i) {
        auto port_int = *ports->at(i).value<uint16_t>();
        std::string ip = absl::StrFormat("localhost:%d", port_int);
        port_map[i] = ip;
    }

    auto log_paths = parsed["log_paths"].as_array();
    std::map<int, std::string> log_path_map;
    for (auto i = 0; i < log_paths->size(); ++i) {
        log_path_map[i] = *log_paths->at(i).value<std::string>();
    }

    uint16_t timeout_min = *parsed["timeout_min"].value<uint16_t>();
    uint16_t timeout_max = *parsed["timeout_max"].value<uint16_t>();

    return ClusterConfig(rank, timeout_min, timeout_max, port_map, log_path_map);
}

std::vector<std::string> ClusterConfig::other_ports() const {
    std::vector<std::string> ports;
    for (auto [rank, value]:ports_) {
        if (rank != rank_) {
            ports.push_back(value);
        }
    }
    return ports;
}