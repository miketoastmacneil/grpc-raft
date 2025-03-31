
find_package(Catch2 3 REQUIRED)

add_executable(Consensus_Tests TimerTest.cpp
        ModuleTests.cpp)
target_link_libraries(Consensus_Tests PRIVATE grpc_raft)
target_link_libraries(Consensus_Tests PRIVATE Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(Consensus_Tests)

