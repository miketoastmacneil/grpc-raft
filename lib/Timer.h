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

class Timer: public std::enable_shared_from_this<Timer> {
public:
    Timer();

    ~Timer();

    void Start(milliseconds duration, std::function<void()> callback);

    void Stop();

private:

    void Run();

    milliseconds duration_;
    std::thread thread_;
    bool active_;
    bool kill_timer_;
    std::function<void()> callback_;
};

}
