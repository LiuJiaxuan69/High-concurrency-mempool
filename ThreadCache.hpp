#pragma once

#include "Common.hpp"

class ThreadCache
{
public:
    //申请和释放内存对象
    void *Allocate(size_t byte);
    void Deallocate(void *ptr, size_t byte);
    Object* FetchFromCentralCache(size_t index, size_t byte);
    //释放内存
    void ListTooLong(FreeList &list, size_t byte);
private:
    FreeList _freelists[NFREELIST];
};

inline thread_local ThreadCache *thread_local_data_ = nullptr;