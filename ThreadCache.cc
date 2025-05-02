#include "ThreadCache.hpp"
#include "CentralCache.hpp"

void * ThreadCache::Allocate(size_t byte)
{
    assert(byte <= MAX_BYTES);
    Object *ret = nullptr;
    size_t memlen = SizeClass::RoundUp(byte);
    size_t index = SizeClass::Index(byte);
    if(_freelists[index].Empty())
    {
        ret = FetchFromCentralCache(index, memlen);
    }
    else
    {
        ret = _freelists[index].Pop();
    }
    return ret;
}

void ThreadCache::Deallocate(void *ptr_, size_t byte)
{
    assert(ptr_);
    Object *ptr = reinterpret_cast<Object *>(ptr_);
    assert(byte <= MAX_BYTES);
    size_t index = SizeClass::Index(byte);
    _freelists[index].Push(ptr);

    if(_freelists[index].Size() >= _freelists[index].MaxSize())
    {
        ListTooLong(_freelists[index], byte);
    }
}

void ThreadCache::ListTooLong(FreeList &list, size_t byte)
{
    Object *start = nullptr, *end = nullptr;
    list.PopRange(start, end, list.MaxSize());
    CentralCache::GetInstance()->ReleaseListToSpans(start, byte);
}

Object* ThreadCache::FetchFromCentralCache(size_t index, size_t byte)
{
    //计算实际获取的块数量
    size_t batchNum = std::min(SizeClass::NumMoveSize(byte), _freelists[index].MaxSize());
    if(batchNum == _freelists[index].MaxSize())
    {
        //慢启动算法（近似网络拥塞控制算法）
        if(_freelists[index].MaxSize() < std::min(middle_level, SizeClass::NumMoveSize(byte) >> 1)) _freelists[index].MaxSize() *= 2;
        else _freelists[index].MaxSize() += 1;
    }

    //处理预备数据存放到freelists以及就绪数据返回
    Object *start = nullptr, *end = nullptr;
    size_t actualNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, byte);
    if(start != end)
    {
        _freelists[index].PushRange(start->next, end, actualNum - 1);
    }
    return start;
}