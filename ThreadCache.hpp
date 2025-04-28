#pragma once

#include "Common.hpp"

class ThreadCache
{
public:
    //申请和释放内存对象
    void *Allocate(size_t byte);
    void Deallocate(void *ptr, size_t byte);
    void* FetchFromCentralCache(size_t index, size_t byte);
private:
    FreeList _freelists[NFREELIST];
};

inline thread_local ThreadCache *thread_local_data_ = nullptr;