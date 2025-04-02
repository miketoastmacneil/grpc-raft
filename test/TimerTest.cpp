//
// Created by Mike MacNeil on 3/17/25.
//

#include <catch2/catch_test_macros.hpp>
#include "Timer.h"

TEST_CASE("TimerInvokesCallbackOnTimeout", "[TimerTest]") {
    std::atomic<bool> timed_out(false);
    grpc_raft::Timer timer;

    timer.Start(std::chrono::milliseconds(150),[&timed_out]() {
        timed_out = true;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    CHECK(timed_out);
}

TEST_CASE("TimerDoesNotInvokeOnRest", "[TimerTest]") {
    std::atomic<bool> timed_out(false);
    grpc_raft::Timer timer;

    timer.Start(std::chrono::milliseconds(150),[&timed_out]() {
        timed_out = true;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    timer.Stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    CHECK(!timed_out);
}

TEST_CASE("TimerStoppedTwiceIsIdempotent", "[TimerTest]") {
    std::shared_ptr<std::atomic<bool>> signal =std::make_shared<std::atomic<bool>>(true);
    bool second_invoked = false;
    grpc_raft::Timer timer;


    timer.Start(std::chrono::milliseconds(100),[signal, &timer, &second_invoked]() {
        if (signal) {
            timer.Start(std::chrono::milliseconds(150),[signal, &timer, &second_invoked]() {
                second_invoked = true;
            });
        }
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    CHECK(second_invoked);
}
