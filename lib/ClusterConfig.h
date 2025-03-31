//
// Created by Mike MacNeil on 3/17/25.
//

#pragma once
#include <map>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

/// An immutable view of the current cluster
/// configuration as well as the configuration
/// for this node.
///
/// The rank is set via a command line flag and the other arguments
/// are specified in the `cluster_config.toml` configuration file
/// included as part of the build.
class ClusterConfig {
public:

    /// Helper for constructing from command line arguments.
    static ClusterConfig FromToml(uint16_t rank, const fs::path& path);

    ClusterConfig(uint16_t rank,
        uint16_t timeout_min,
        uint16_t timeout_max,
        std::map<int, std::string> ports,
        std::map<int, std::string> log_paths):rank_{rank},
    timeout_min_{timeout_min},
    timeout_max_{timeout_max},
    ports_{ports},
    log_paths_{log_paths} {}

    uint16_t timeout_min() const {return timeout_min_;}

    uint16_t timeout_max() const {return timeout_max_;}

    uint16_t rank() const {return rank_; }

    std::string port() const {return ports_.at(rank_);}

    std::string log_path() const { return log_paths_.at(rank_); }

    /// Return the addresses of all other nodes in the cluster
    std::vector<std::string> other_ports() const;

private:

    /// The node ID of this node in the cluster
    uint16_t rank_;

    /// Delineate the range in which a random timeout will be selected
    uint16_t timeout_min_;
    uint16_t timeout_max_;

    /// The ports of all nodes in this cluster
    std::map<int, std::string> ports_;
    /// The log paths of all nodes in this cluster.
    std::map<int, std::string> log_paths_;
};
