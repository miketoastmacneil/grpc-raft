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

struct RunTimer {

RunTimer(const std::chrono::milliseconds &duration,
    TimerCallback callback,
    BoolRef interrupt): duration_(duration), callback_(callback), interrupt_(interrupt) {}

void operator()() {
    std::unique_lock<std::mutex> lock(mutex_);
    auto interrupt_copy = interrupt_;
    condition_variable_.wait_for(lock, duration_, [interrupt_copy]() {
        return (*interrupt_copy).load();
    });

    if (!(*interrupt_copy).load()) {
        callback_();
    }
}

std::chrono::milliseconds duration_;
TimerCallback callback_;
BoolRef interrupt_;
};

class Timer &Timer::operator=(Timer &&other) noexcept {
    duration_ = std::move(other.duration_);
    callback_ = std::move(other.callback_);
    thread_ = std::move(other.thread_);
    interrupt_ = std::move(other.interrupt_);
    running_ = std::move(other.running_);
    reset();
    return *this;
}


Timer::~Timer() {
    reset();
}

void Timer::start() {
    if (!(*running_)) {
        *running_ = true;
        *interrupt_ = false;
        RunTimer runner(duration_, callback_, interrupt_);
        thread_ = std::thread(std::move(runner));
    }
}

void Timer::reset() {
    if (!running_) {
        return;
    }
    if (*running_) {
        *running_ = false;
        *interrupt_ = true;
        if (thread_.joinable()) {
            thread_.join();
        }
    }
}


}