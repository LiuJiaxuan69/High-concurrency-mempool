#pragma once

#include "ThreadCache.hpp"
#include "PageCache.hpp"
#include <format>
#include <syncstream>

inline void* ConcurrentAlloc(size_t byte)
{
    if(byte > MAX_BYTES)
    {
        size_t alignbyte = SizeClass::RoundUp(byte);
        size_t pageNum = alignbyte >> PAGE_SHIFT;
        PageCache::GetInstance()->mtx.lock();
        Span *span = PageCache::GetInstance()->NewSpan(pageNum);
        span->isusing = true;
        span->objSize = alignbyte;
        PageCache::GetInstance()->mtx.unlock();
        void* ptr = (void*)(span->pageId << PAGE_SHIFT);
		return ptr;
    }
    if(thread_local_data_ == nullptr)
    {
        thread_local_data_ = new ThreadCache();
    }
    // auto output1 = std::this_thread::get_id();
    // std::cout << std::format("{}:{}\n", output1, (void*)thread_local_data_);
    // std::osyncstream(std::cout) << std::this_thread::get_id() << ":" << thread_local_data_ << std::endl;
    return thread_local_data_->Allocate(byte);
}

inline void ConcurrentFree(void *ptr)
{
    Span *span = PageCache::GetInstance()->MapAddrToSpan(ptr);
    size_t byte = span->objSize;
    if(byte > MAX_BYTES)
    {
        PageCache::GetInstance()->mtx.lock();
        PageCache::GetInstance()->ReleaseSpanToPageCache(span);
        PageCache::GetInstance()->mtx.unlock();
        return;
    }
    thread_local_data_->Deallocate(ptr, byte);
}