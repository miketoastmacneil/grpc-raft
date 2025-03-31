//
// Created by Mike MacNeil on 3/17/25.
//

#pragma once

#include <cstdint>
#include <functional>
#include <iostream>
#include <chrono>
#include <thread>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace grpc_raft {

class Timer {
public:
    Timer() = default;

    Timer& operator=(Timer&& other) noexcept;

    Timer(std::chrono::milliseconds duration, std::function<void()> callback)
        : duration_(duration),
        callback_(callback),
        interrupt_(std::make_shared<std::atomic<bool>>(false)),
        running_(std::make_unique<std::atomic<bool>>(false)) {}

    ~Timer();

    void start();

    void reset();

private:

    std::chrono::milliseconds duration_;
    std::function<void()> callback_;
    std::thread thread_;
    std::shared_ptr<std::atomic<bool>> interrupt_;
    std::unique_ptr<std::atomic<bool>> running_;
};

}
