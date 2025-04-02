
set(RAFT_HDRS
    ClusterConfig.h
    ConsensusModule.h
    Timer.h
)

set(RAFT_SRCS
    ClusterConfig.cpp
    ConsensusModule.cpp
    Timer.cpp
    PersistentStateManager.cpp
    PersistentStateManager.h
)

add_library(
        grpc_raft
        STATIC
        ${RAFT_HDRS}
        ${RAFT_SRCS}
)
target_include_directories(grpc_raft PUBLIC ${tomlplusplus_SOURCE_DIR})
target_link_libraries(grpc_raft PUBLIC
        protolib
        gRPC::grpc++
        gRPC::grpc++_reflection
        absl::flags
        absl::flags_parse
        tomlplusplus::tomlplusplus
        nlohmann_json::nlohmann_json
)