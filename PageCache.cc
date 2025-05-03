#include "PageCache.hpp"

Span *PageCache::NewSpan(size_t k)
{
    // assert(k >= 1 && k <= NPAGES);
    if (k > NPAGES)
    {
        void *ptr = VirtualAlloc(0, k << PAGE_SHIFT, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (ptr == nullptr)
            throw std::bad_alloc();
        // 将该内存分成两份
        Span *newSpan = GetSpanPool().New();
        newSpan->n = k;
        newSpan->pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
        PageIdToSpan[newSpan->pageId] = newSpan;
        return newSpan;
    }
    // 检查当前Span下有没有可用的PAGE
    if (!_spanLists[k].Empty())
    {
        Span *kspan = _spanLists[k].PopFront();
        for (PAGE_ID i = 0; i < kspan->n; ++i)
        {
            PageIdToSpan[kspan->pageId + i] = kspan;
        }
        return kspan;
    }
    // 否则去后面查看是否有非空Span可使用来分割
    for (int i = k + 1; i <= NPAGES; ++i)
    {
        if (_spanLists[i].Empty())
            continue;
        // 发现非空Span，可以将其分割为两份Span
        Span *span = _spanLists[i].PopFront();
        Span *kspan = GetSpanPool().New();
        kspan->pageId = span->pageId;
        kspan->n = k;
        span->pageId += k;
        span->n -= k;
        _spanLists[i - k].PushFront(span);

        // 存放Span的首尾PageId用于PageCache合并Span块
        PageIdToSpan[span->pageId] = span;
        PageIdToSpan[span->pageId + span->n - 1] = span;

        // 建立返回的kspan其PageId与自身映射的地址关系
        for (PAGE_ID i = 0; i < kspan->n; ++i)
        {
            PageIdToSpan[kspan->pageId + i] = kspan;
        }
        return kspan;
    }

    // 此时说明没有大块Span供以分割，此时需要向上层（系统）索要一块堆空间
    void *ptr = VirtualAlloc(0, NPAGES << PAGE_SHIFT, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (ptr == nullptr)
        throw std::bad_alloc();
    // 将该内存分成两份
    Span *newSpan = GetSpanPool().New();
    newSpan->n = NPAGES;
    newSpan->pageId = (PAGE_ID)ptr >> PAGE_SHIFT;

    // 直接从newSpan中切割
    Span *kspan = GetSpanPool().New();
    kspan->pageId = newSpan->pageId;
    kspan->n = k;
    newSpan->pageId += k;
    newSpan->n -= k;

    PageIdToSpan[newSpan->pageId] = newSpan;
    PageIdToSpan[newSpan->pageId + newSpan->n - 1] = newSpan;

    for (PAGE_ID i = 0; i < kspan->n; ++i)
    {
        PageIdToSpan[kspan->pageId + i] = kspan;
    }
    _spanLists[newSpan->n].PushFront(newSpan);
    return kspan;
}

Span *PageCache::MapAddrToSpan(void *addr)
{
    PAGE_ID pageId = (PAGE_ID)addr >> PAGE_SHIFT;
    std::unique_lock<Mutex> lock(mtx);
    auto it = PageIdToSpan.find(pageId);
    if (it == PageIdToSpan.end())
        assert(false);
    return it->second;
}

void PageCache::ReleaseSpanToPageCache(Span *span)
{
    // 查找span前面可以合并的Span
    if(span->n > NPAGES)
    {
        void *ptr = (void*)(span->pageId << PAGE_SHIFT);
        VirtualFree(ptr, 0, MEM_RELEASE);
        GetSpanPool().Delete(span);
        return;
    }
    while (true)
    {
        PAGE_ID prev = span->pageId - 1;
        auto it = PageIdToSpan.find(prev);

        // 如果找不到或者找到的页正在被使用或者找到的页大小合并后总大小大于NPAGES,则退出循环
        if (it == PageIdToSpan.end() || it->second->isusing || span->n + it->second->n > 128)
            break;

        // 否则开始合并
        Span *prevspan = it->second;
        span->pageId = prevspan->pageId;
        span->n += prevspan->n;

        // 删除被合并的span
        _spanLists[prevspan->n].Erase(prevspan);
        GetSpanPool().Delete(prevspan);
    }
    // 查找span后面可以合并的Span
    while (true)
    {
        PAGE_ID next = span->pageId + span->n;
        auto it = PageIdToSpan.find(next);

        // 如果找不到或者找到的页正在被使用或者找到的页大小合并后总大小大于NPAGES,则退出循环
        if (it == PageIdToSpan.end() || it->second->isusing || span->n + it->second->n > NPAGES)
            break;

        // 否则开始合并
        Span *nextspan = it->second;
        span->n += nextspan->n;

        // 删除被合并的span
        _spanLists[nextspan->n].Erase(nextspan);
        GetSpanPool().Delete(nextspan);
    }

    // 修改合并好的 span 的 状态
    span->isusing = false;
    _spanLists[span->n].PushFront(span);
    PageIdToSpan[span->pageId] = span;
    PageIdToSpan[span->pageId + span->n - 1] = span;
}