//
// Created by Mike MacNeil on 3/17/25.
//

#include <catch2/catch_test_macros.hpp>
#include "Timer.h"

TEST_CASE("TimerInvokesCallbackOnTimeout", "[TimerTest]") {
    std::atomic<bool> timed_out(false);
    grpc_raft::Timer timer(std::chrono::milliseconds(150),[&timed_out]() {
        timed_out = true;
    });

    timer.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    CHECK(timed_out);
}

TEST_CASE("TimerDoesNotInvokeOnRest", "[TimerTest]") {
    std::atomic<bool> timed_out(false);
    grpc_raft::Timer timer(std::chrono::milliseconds(150),[&timed_out]() {
        timed_out = true;
    });

    timer.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    timer.Reset();

    CHECK(!timed_out);
}

TEST_CASE("TimerStoppedTwiceIsIdempotent", "[TimerTest]") {
    std::atomic<bool> timed_out(false);
    grpc_raft::Timer timer(std::chrono::milliseconds(150),[&timed_out]() {
        timed_out = true;
    });

    timer.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    timer.Reset();
    timer.Reset();

    CHECK(!timed_out);
}
