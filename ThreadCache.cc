#include "ThreadCache.hpp"
#include "CentralCache.hpp"

void * ThreadCache::Allocate(size_t byte)
{
    assert(byte <= MAX_BYTES);
    void *ret = nullptr;
    size_t memlen = SizeClass::RoundUp(byte);
    size_t index = SizeClass::Index(byte);
    if(_freelists[index].Empty())
    {
        ret = FetchFromCentralCache(index, byte);
    }
    else
    {
        ret = _freelists[index].Pop();
    }
    return ret;
}

void ThreadCache::Deallocate(void *ptr, size_t byte)
{
    assert(ptr);
    assert(byte <= MAX_BYTES);
    size_t index = SizeClass::Index(byte);
    _freelists[index].Push(ptr);
}

void* ThreadCache::FetchFromCentralCache(size_t index, size_t byte)
{
    size_t batchNum = std::min(SizeClass::NumMoveSize(byte), _freelists[index].MaxSize());
    if(batchNum == _freelists[index].MaxSize())
    {
        //慢启动算法（近似网络拥塞控制算法）
        if(_freelists[index].MaxSize() < middle_level) _freelists[index].MaxSize() *= 2;
        else _freelists[index].MaxSize() += 1;
    }

    //处理预备数据存放到freelists以及就绪数据返回
    void *start = nullptr, *end = nullptr;
    size_t actualNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, byte);
    if(start == end) assert(start == end);
    else
    {
        _freelists[index].PushRange(NextObj(start), end);
    }
    return start;
}