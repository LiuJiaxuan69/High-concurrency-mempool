#pragma once
#include <chrono>
#include <atomic>
#include <stdexcept>

// 全局累计时间（线程安全）
inline std::atomic<uint64_t> g_total_elapsed_us{0};

class Timer {
public:
    // 模式选择
    enum class Mode { Auto, Manual };

    // 构造函数：Auto 模式自动开始，Manual 模式需手动 Start()
    explicit Timer(Mode mode = Mode::Auto) : mode_(mode) {
        if (mode_ == Mode::Auto) Start();
    }

    // 析构函数：Auto 模式自动结束并累计时间
    ~Timer() {
        if (mode_ == Mode::Auto && is_running_) {
            End();
        }
    }

    // 开始计时
    void Start() {
        if (is_running_) {
            throw std::runtime_error("Timer already running!");
        }
        start_time_ = std::chrono::high_resolution_clock::now();
        is_running_ = true;
    }

    // 结束计时，返回本次耗时（微秒），并累加到全局变量
    uint64_t End() {
        if (!is_running_) {
            throw std::runtime_error("Timer not started!");
        }
        auto end_time = std::chrono::high_resolution_clock::now();
        auto elapsed_us = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end_time - start_time_
        ).count();
        
        g_total_elapsed_us += elapsed_us;
        is_running_ = false;
        return elapsed_us;
    }

    // 禁止拷贝和移动
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

private:
    Mode mode_;
    bool is_running_ = false;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time_;
};
