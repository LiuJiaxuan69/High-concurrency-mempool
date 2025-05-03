#pragma once

#include "Common.hpp"

class CentralCache
{
public:
    static CentralCache* GetInstance()
    {
        return &_centralCache;
    }
    size_t FetchRangeObj(Object *&start, Object *&end, size_t batchNum, size_t byte);
    Span *GetOneSpan(SpanList &list, size_t byte);
    void ReleaseListToSpans(Object *start, size_t byte);
    auto GetMutexTime()
    {
        std::chrono::nanoseconds ret;
        for(int i = 0; i < NFREELIST; ++i) ret += spanLists[i].mtx.GetTime();
        return ret;
    }
public:
    SpanList spanLists[NFREELIST];
private:
    static  CentralCache _centralCache;

private:
    CentralCache() = default;
    CentralCache(const CentralCache&) = delete;
    //防止new/delete误操作
    void* operator new(size_t) = delete;
    void* operator new[](size_t) = delete;
    void operator delete(void*) = delete;
    void operator delete[](void*) = delete;
};

inline CentralCache CentralCache::_centralCache;