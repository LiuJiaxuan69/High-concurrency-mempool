#include "CentralCache.hpp"
#include "PageCache.hpp"

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
    Span *it = list.Begin();
    //若找得到剩余的的Span则直接返回
    while(it != list.End())
    {
        if(it != nullptr) return it;
        it = it->next;
    }

    //先释放桶锁，别的线程可能正在释放资源
    list.mtx.unlock();

    //需要向上层PageCache申请Page进行分割
    PageCache::GetInstance()->mtx.lock();
    Span *span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(byte));
    PageCache::GetInstance()->mtx.unlock();

    //对页切片
    char *start = (char*)(span->pageId << PAGE_SHIFT);
    size_t size = span->n << PAGE_SHIFT;
    char *end = start + size;

    span->freelist = start;
    start += byte;
    void *tail = span->freelist;
    while(start < end)
    {
        NextObj(tail) = start;
        tail = start;
        start += byte;
    }

    //回去之前上锁
    list.mtx.lock();
    
    list.Insert(list.Begin(), span);
    return span;
}