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
    Timer() = default;

    ~Timer();

    void Start(milliseconds duration, std::function<void()> callback);

    void Stop();

private:

    std::thread thread_;
    std::shared_ptr<std::atomic<bool>> interrupt_;
};

}
