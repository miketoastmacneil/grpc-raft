
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <toml++/toml.hpp>

#include <iostream>
#include <filesystem>
#include <memory>
#include <string>

#include "ConsensusModule.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"

#include "proto/Messages.pb.h"
#include "proto/Messages.grpc.pb.h"

#include "lib/ClusterConfig.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using raft::VoteRequest;
using raft::VoteResponse;
using raft::VoteRequest;
using raft::VoteResponse;

ABSL_FLAG(uint16_t, rank, 1, "Node rank in the cluster");
ABSL_FLAG(std::string, cluster_config, "cluster_config.toml", "Configuration for the cluster");

void RunServer(ClusterConfig config) {
    std::string server_address = config.port();
    std::shared_ptr<ConsensusModule> service = std::make_shared<ConsensusModule>(config);

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(service.get());

    std::unique_ptr<Server> server(builder.BuildAndStart());
    // Sleep for a minute before we try and connect to peers.
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    if (service->ConnectToPeers()) {
        std::cout << "Connected to all peers" << std::endl;
    }
    service->StartElectionTimer();
    server->Wait();
}

int main(int argc, char** argv) {

    absl::ParseCommandLine(argc, argv);
    uint16_t rank = absl::GetFlag(FLAGS_rank);
    std::string cluster_config = absl::GetFlag(FLAGS_cluster_config);
    try {
        auto config = ClusterConfig::FromToml(rank-1, cluster_config);
        RunServer(config);
    } catch (const toml::parse_error &e) {
        std::cerr << e.what() << std::endl;
    }

    // RunServer(absl::GetFlag(FLA));
    return 0;
}