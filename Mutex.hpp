#pragma once

#include <iostream>
#include <mutex>
#include <atomic>
#include "Timer.hpp"


class Mutex
{
    public:
    void lock()
    {
        // Timer timer;
        _mtx.lock();
        // for (
        //     auto t = total_time.load();
        //     !total_time.compare_exchange_strong(t, t + timer.GetCurrentDuration());
        //     t = total_time.load());
    }
    void unlock()
    {
        // Timer timer;
        _mtx.unlock();
        // for (
        //     auto t = total_time.load();
        //     !total_time.compare_exchange_strong(t, t + timer.GetCurrentDuration());
        //     t = total_time.load());
    }
    auto GetTime()
    {
        return total_time.load();
    }
    private:
    std::mutex _mtx;
    std::atomic<std::chrono::nanoseconds> total_time;
    // Timer& timer{timer::GetInstance()};

};