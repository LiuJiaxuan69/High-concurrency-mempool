#include "CentralCache.hpp"

size_t CentralCache::FetchRangeObj(void *&start, void *&end, size_t batchNum, size_t byte)
{
    size_t index = SizeClass::Index(byte);
    spanLists[index].mtx.lock();

    Span *span = GetOneSpan(spanLists[index], byte);
    assert(span);
    assert(span->freelist);

    start = span->freelist;
    end = start;
    size_t i = 0, actual_num = 1;
    //最多获取到链表尾部
    while(i < batchNum - 1 && NextObj(end))
    {
        end = NextObj(end);
        ++i;
        ++actual_num;
    }
    span->freelist = NextObj(end);
    NextObj(end) = nullptr;
    span->useCount += actual_num;
    spanLists[index].mtx.unlock();
    return actual_num;
}
Span* CentralCache::GetOneSpan(SpanList &list, size_t byte)
{
    return nullptr;
}