cmake_minimum_required(VERSION 3.30)
project(grpc_raft CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_PREFIX_PATH "/Users/mikemacneil/.local")

include(FetchContent)
FetchContent_Declare(
        tomlplusplus
        GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
        GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(tomlplusplus)

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(nlohmann_json 3.11.3 REQUIRED)

add_library(protolib OBJECT proto/Messages.proto)
target_link_libraries(protolib PUBLIC protobuf::libprotobuf gRPC::grpc++)
target_include_directories(protolib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate(
        TARGET protolib
        LANGUAGE cpp
)

protobuf_generate(
        TARGET protolib
        LANGUAGE grpc
        GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
        PLUGIN "protoc-gen-grpc=\$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
)

configure_file(lib/cluster_config.toml lib/cluster_config.toml COPYONLY)

include_directories(lib)
add_subdirectory(lib)

add_executable(grpc_raft_server
        main.cpp
)
target_link_libraries(grpc_raft_server PUBLIC protolib grpc_raft)

add_subdirectory(test)
