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
    std::condition_variable timer_cv_;
    std::thread thread_;
}

using BoolRef = const std::shared_ptr<std::atomic_bool>;
using DurationRef = const std::shared_ptr<std::chrono::milliseconds>;
using TimerCallback = std::function<void()>;
using CallbackRef = const std::shared_ptr<std::function<void()>>;

void Run(DurationRef duration, CallbackRef callback, BoolRef active, BoolRef kill, BoolRef interrupt_acknowledgement) {

    auto kill_predicate = [=]() {
        if (kill) {
            return (*kill).load();
        }
        // If kill is now null, return
        return true;
    };

    while (!kill_predicate()) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (interrupt_acknowledgement->load()) {
            interrupt_acknowledgement->notify_one();
        }

        auto wait_for_start_predicate = [=]() {
            return active->load() || kill->load();
        };

        notifier_.wait(lock, wait_for_start_predicate);

        if (kill_predicate()) break;
        auto interrupt_invoked = [=]() {
            return !(active->load())|| (kill->load());
        };

        timer_cv_.wait_for(lock, *duration, interrupt_invoked);
        if (interrupt_invoked()) {
            // Signal back that we were interrupted and continuing.
            interrupt_acknowledgement->store(true);
            continue;
        }

        if (callback) {
            // Lock is re-acquired at the next iteration of
            // the loop
            lock.unlock();
            callback->operator()();
        }
    }

}


Timer::Timer() {
    duration_ = std::make_shared<milliseconds>(0);
    callback_ = std::make_shared<std::function<void()>>();
    active_ = std::make_shared<std::atomic<bool>>(false);
    kill_timer_ = std::make_shared<std::atomic<bool>>(false);
    interrupt_acknowledgement_ = std::make_shared<std::atomic<bool>>(false);
}

Timer::~Timer() {
    active_->store(true);
    kill_timer_->store(true);
    notifier_.notify_all();
    timer_cv_.notify_all();
    if (thread_.joinable()) {
        thread_.join();
    }
}

void Timer::Start(milliseconds duration, TimerCallback callback) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!thread_.joinable()) {
        thread_ = std::thread(Run, duration_, callback_, active_, kill_timer_, interrupt_acknowledgement_);
    }
    if (active_->load()) {
        // Yield the lock to Stop function.
        lock.unlock();
        Stop();
        interrupt_acknowledgement_->wait(false);
        lock.lock();
    }
    interrupt_acknowledgement_->store(false);
    *duration_ = duration;
    *callback_ = callback;
    active_->store(true);
    notifier_.notify_one();
}

void Timer::Stop() {
    active_->store(false);
    timer_cv_.notify_one();
}

}

