#pragma once

#include "Common.hpp"

//饿汉模式
class PageCache
{
public:
    static PageCache *GetInstance()
    {
        return &_pageCache;
    }
    Span *NewSpan(size_t k);
    Span *MapAddrToSpan(void *addr);
    void ReleaseSpanToPageCache(Span *span);
private:
    PageCache() = default;
    PageCache(const PageCache&) = delete;
    //防止new/delete误操作
    void* operator new(size_t) = delete;
    void* operator new[](size_t) = delete;
    void operator delete(void*) = delete;
    void operator delete[](void*) = delete;
private:
    static PageCache _pageCache;
public:
    std::mutex mtx;
    SpanList _spanLists[NPAGES + 1];
    std::unordered_map<PAGE_ID, Span*> PageIdToSpan;
};

inline PageCache PageCache::_pageCache;