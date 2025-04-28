#pragma once

#include "ThreadCache.hpp"

inline void* ConcurrentAlloc(size_t byte)
{
    if(thread_local_data_ == nullptr)
    {
        thread_local_data_ = new ThreadCache();
    }
    std::cout << std::this_thread::get_id() << ":" << thread_local_data_ << std::endl;
    return thread_local_data_->Allocate(byte);
}

inline void* ConcurrentFree(void *ptr)
{
    return nullptr;
}