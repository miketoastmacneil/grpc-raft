//
// Created by Mike MacNeil on 3/17/25.
//

#include "Timer.h"

namespace grpc_raft {

namespace {
std::mutex mutex_;
std::condition_variable condition_variable_;
}

using BoolRef = const std::shared_ptr<std::atomic_bool>;
using TimerCallback = std::function<void()>;

Timer::~Timer() {
    if (thread_.joinable()) {
        thread_.detach();
    }
}

void Timer::Start(milliseconds duration, TimerCallback callback) {
    Stop();
    interrupt_ = std::make_shared<std::atomic_bool>(false);
    auto interrupt_copy = interrupt_;
    thread_ = std::thread([=]() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_variable_.wait_for(lock, duration, [interrupt_copy]() {
            return (*interrupt_copy).load();
        });
        if (!(*interrupt_copy).load()) {
            callback();
        }
    });
}

void Timer::Stop() {
    if (interrupt_) {
        *interrupt_ = true;
    }
    if (thread_.joinable()) {
        thread_.detach();
    }
}

}

