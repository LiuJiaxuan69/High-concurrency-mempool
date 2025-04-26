#pragma once

#include "ThreadCache.hpp"

void * ThreadCache::Allocate(size_t byte)
{
    assert(byte <= MAX_BYTES);
    void *ret = nullptr;
    size_t memlen = SizeClass::RoundUp(byte);
    size_t index = SizeClass::Index(byte);
    if(_freelists[index]->Empty())
    {
        ret = FetchFromCentralCache(index, byte);
    }
    else
    {
        ret = _freelists[index]->Pop();
    }
    return ret;
}

void ThreadCache::Deallocate(void *ptr, size_t byte)
{
    assert(ptr);
    assert(byte <= MAX_BYTES);
    size_t index = SizeClass::Index(byte);
    _freelists[index]->Push(ptr);
}