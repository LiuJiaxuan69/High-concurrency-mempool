#include "PageCache.hpp"

Span *PageCache::NewSpan(size_t k)
{
    assert(k >= 1 && k <= NPAGES);
    // 检查当前Span下有没有可用的PAGE
    if (!_spanLists[k].Empty())
    {
        return _spanLists[k].PopFront();
    }
    // 否则去后面查看是否有非空Span可使用来分割
    for (int i = k + 1; i <= NPAGES; ++i)
    {
        if (_spanLists[i].Empty())
            continue;
        // 发现非空Span，可以将其分割位两份Span
        Span *span = _spanLists[i].PopFront();
        Span *kspan = new Span();
        kspan->pageId = span->pageId;
        kspan->n = k;
        span->pageId += k;
        span->n -= k;

        _spanLists[i - k].PushFront(span);
        return kspan;
    }

    // 此时说明没有大块Span供以分割，此时需要向上层（系统）索要一块堆空间
    void *ptr = VirtualAlloc(0, NPAGES << PAGE_SHIFT, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (ptr == nullptr)
        throw std::bad_alloc();
    // 将该内存分成两份
    Span *newSpan = new Span();
    newSpan->n = NPAGES;
    newSpan->pageId = (PAGE_ID)ptr >> PAGE_SHIFT;

    //直接从newSpan中切割
    Span *kspan = new Span();
    kspan->pageId = newSpan->pageId;
    kspan->n = k;
    newSpan->pageId += k;
    newSpan->n -= k;
    _spanLists[newSpan->n].PushFront(newSpan);
    return kspan;
}