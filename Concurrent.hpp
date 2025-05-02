#pragma once

#include "ThreadCache.hpp"
#include <format>
#include <syncstream>

inline void* ConcurrentAlloc(size_t byte)
{
    if(thread_local_data_ == nullptr)
    {
        thread_local_data_ = new ThreadCache();
    }
    // auto output1 = std::this_thread::get_id();
    // std::cout << std::format("{}:{}\n", output1, (void*)thread_local_data_);
    // std::osyncstream(std::cout) << std::this_thread::get_id() << ":" << thread_local_data_ << std::endl;
    return thread_local_data_->Allocate(byte);
}

template<class T, class... Args>
T* testAlloc(Args&&... args)
{
    T* ret = (T*)ConcurrentAlloc(sizeof(T));
    new (ret) T(std::forward<Args>(args)...);
    return ret;
}

inline void* ConcurrentFree(void *ptr,size_t byte)
{
    thread_local_data_->Deallocate(ptr, byte);
    return nullptr;
}