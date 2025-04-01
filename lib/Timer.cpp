//
// Created by Mike MacNeil on 3/17/25.
//

#include "Timer.h"

namespace grpc_raft {

namespace {

    // This mutex guards the conditional variable for
    // signalling.
    std::mutex mutex_;
    std::condition_variable notifier_;
    std::condition_variable timer_condition_variable_;
}

using BoolRef = const std::shared_ptr<std::atomic_bool>;
using TimerCallback = std::function<void()>;

Timer::Timer() {
    duration_ = std::chrono::milliseconds(0);
    callback_ = nullptr;
    kill_timer_ = false;
    active_ = false;

    thread_ = std::thread([this]() {
        // Capturing self keeps this alive, otherwise
        this->Run();
    });
}

Timer::~Timer() {
    kill_timer_ = true;
    if (thread_.joinable()) {
        thread_.join();
    }
}

void Timer::Start(milliseconds duration, TimerCallback callback) {
    Stop();
    std::lock_guard<std::mutex> lock(mutex_);
    duration_ = duration;
    callback_ = callback;
    active_ = true;
    notifier_.notify_one();
}

void Timer::Stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    active_ = false;
    notifier_.notify_one();
}

void Timer::Run() {
    while (!kill_timer_){
        std::unique_lock<std::mutex> lock(mutex_);

        // This will exist here since we've passed by shared.
        // Modifications to kill_timer_ as well as
        // active are guarded by locks
        auto wait_for_start_predicate = [this]() {
            return active_ || kill_timer_;
        };

        notifier_.wait(lock, wait_for_start_predicate);

        if (kill_timer_) {
            break;
        }

        auto interrupt_invoked = [this]() {
            return !active_ || kill_timer_;
        };

        timer_condition_variable_.wait_for(lock, duration_, interrupt_invoked);

        if (!interrupt_invoked() || callback_) {
            active_ = false;
            // Lock is re-acquired at the next iteration of
            // the loop
            lock.unlock();
            callback_();
        }
    }
}

}

