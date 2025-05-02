#include "CentralCache.hpp"
#include "PageCache.hpp"

size_t CentralCache::FetchRangeObj(Object *&start, Object *&end, size_t batchNum, size_t byte)
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
    while(i < batchNum - 1 && end->next != nullptr)
    {
        end = end->next;
        ++i;
        ++actual_num;
    }
    span->freelist = end->next;
    end->next = nullptr;
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
        if(it->freelist != nullptr) return it;
        it = it->next;
    }

    //先释放桶锁，别的线程可能正在释放资源
    list.mtx.unlock();

    //需要向上层PageCache申请Page进行分割
    PageCache::GetInstance()->mtx.lock();
    Span *span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(byte));
    span->isusing = true;
    PageCache::GetInstance()->mtx.unlock();

    //对页切片
    std::byte *start = (std::byte*)(span->pageId << PAGE_SHIFT);
    size_t size = span->n << PAGE_SHIFT;
    std::byte *end = start + size;

    span->freelist = reinterpret_cast<Object *>(start);
    start += byte;
    Object *tail = span->freelist;
    while(start < end)
    {
        tail->next = reinterpret_cast<Object *>(start);
        tail = tail->next;
        start += byte;
    }
    tail->next = nullptr;
    //回去之前上锁
    list.mtx.lock();
    
    list.PushFront(span);
    return span;
}

void CentralCache::ReleaseListToSpans(Object *start, size_t byte)
{
    size_t index = SizeClass::Index(byte);
    spanLists[index].mtx.lock();
    while(start)
    {
        Object *next = start->next;
        //获取映射的*span并归还
        PageCache::GetInstance()->mtx.lock();
        Span *span = PageCache::GetInstance()->MapAddrToSpan(start);
        PageCache::GetInstance()->mtx.unlock();
        start->next = span->freelist;
        span->freelist = start;
        span->useCount--;
        
        //若userCount为0，则说明所有内存块均已归还，此时可以将内存块交给上层
        if(span->useCount == 0)
        {
            spanLists[index].Erase(span);

            span->freelist = nullptr;
            span->next = nullptr;
            span->prev = nullptr;

            spanLists[index].mtx.unlock();
            PageCache::GetInstance()->mtx.lock();
            PageCache::GetInstance()->ReleaseSpanToPageCache(span);
            PageCache::GetInstance()->mtx.unlock();
            spanLists[index].mtx.lock();
        }

        start = next;
    }
    spanLists[index].mtx.unlock();
}
