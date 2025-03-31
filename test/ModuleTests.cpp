//
// Created by Mike MacNeil on 3/23/25.
//

#include <catch2/catch_test_macros.hpp>
#include "ClusterConfig.h"
#include "ConsensusModule.h"

TEST_CASE("InitializedModuleIsAFollower","[ModuleTests]") {


    ClusterConfig config{
        0, 150, 300, {{0, "50051"}}, {{0, "/tmp/log_50051.log"}}
    };

    auto module = ConsensusModule(config);


}