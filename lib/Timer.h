//
// Created by Mike MacNeil on 3/17/25.
//

#pragma once

#include <functional>
#include <chrono>
#include <thread>
#include <condition_variable>

namespace grpc_raft {

using milliseconds = std::chrono::milliseconds;

class Timer {
public:
    Timer();

    ~Timer();

    void Start(milliseconds duration, std::function<void()> callback);

    void Stop();

private:

    /// Each shared reference is passed to the run loop
    std::shared_ptr<milliseconds> duration_;
    std::shared_ptr<std::atomic<bool>> active_;
    std::shared_ptr<std::atomic<bool>> kill_timer_;
    std::shared_ptr<std::atomic<bool>> interrupt_acknowledgement_;
    std::shared_ptr<std::function<void()>> callback_;

    // This mutex guards the conditional variable for
    // signalling.
    std::shared_ptr<std::mutex> mutex_;
    std::shared_ptr<std::condition_variable> notifier_;
    std::shared_ptr<std::condition_variable> timer_cv_;
    std::shared_ptr<std::thread> thread_;
};

}
