#pragma once

#include <iostream>
#include <vector>
#include <Windows.h>
#include <memoryapi.h>
#include <assert.h>
#include <thread>
#include <mutex>
using std::cin;
using std::cout;

//使得页编号适配不同的windows环境
#ifdef _WIN64
    using PAGE_ID = unsigned long long;
#elif _WIN32
    using PAGE_ID = size_t;
#endif

const int MAX_BYTES = 256 * 1024;
const int NFREELIST = 208;

inline void* &NextObj(void *obj)
{
    return *(void**)obj;
}

class FreeList
{
public:
    void Push(void *obj)
    {
        assert(obj);
        NextObj(obj) = _freelist;
        _freelist = obj;
        
    }
    void* Pop()
    {
        assert(_freelist);
        void *obj = _freelist;
        _freelist = NextObj(obj);
        return obj;
    }
    bool Empty()
    {
        return _freelist == nullptr;
    }
private:
    void *_freelist = nullptr;
};

class SizeClass
{
public:
    // 整体控制在最多10%左右的内碎片浪费
	// [1,128]					8byte对齐	    freelist[0,16)
	// [128+1,1024]				16byte对齐	    freelist[16,72)
	// [1024+1,8*1024]			128byte对齐	    freelist[72,128)
	// [8*1024+1,64*1024]		1024byte对齐     freelist[128,184)
	// [64*1024+1,256*1024]		8*1024byte对齐   freelist[184,208)
    //计算对其后的byte
    static inline size_t RoundUp(size_t byte)
    {
        if(byte <= 128)
        {
            return _RoundUp(byte, 8);
        }
        else if(byte <= 1024)
        {
            return _RoundUp(byte, 16);
        }
        else if(byte <= 8 * 1024)
        {
            return _RoundUp(byte, 128);
        }
        else if(byte <= 64 * 1024)
        {
            return _RoundUp(byte, 1024);
        }
        else if(byte <= 256 * 1024)
        {
            return _RoundUp(byte, 8 * 1024);
        }
        else
        {
            assert(false);
        }
    }
    static inline size_t _RoundUp(size_t byte, size_t alignNum)
    {
        return (byte + alignNum - 1) & (~(alignNum - 1));
    }

    //计算数据存放对应的桶
    static inline size_t Index(size_t byte)
    {
        //前缀桶数
        int pre_sum[5] = {0, 16, 72, 128, 184};
        if(byte <= 128)
        {
            return _Index(byte, 3) + pre_sum[0];
        }
        else if(byte <= 1024)
        {
            return _Index(byte - 128, 4) + pre_sum[1];
        }
        else if(byte <= 8 * 1024)
        {
            return _Index(byte - 1024, 7) + pre_sum[2];
        }
        else if(byte <= 64 * 1024)
        {
            return _Index(byte - 8 * 1024, 10) + pre_sum[3];
        }
        else if(byte <= 256 * 1024)
        {
            return _Index(byte - 64 * 1024, 13) + pre_sum[4];
        }
        else
        {
            assert(false);
        }
    }
    static inline size_t _Index(size_t byte, size_t alignBit)
    {
        return ((byte + (1 << alignBit) - 1) >> alignBit) - 1;
    }
};

struct Span
{
    PAGE_ID pageId; //页编号
    Span *prev = nullptr;
    Span *next = nullptr;
    size_t n = 0; //页数量
    void *freelist = nullptr;
};

class SpanList
{
public:
    SpanList()
    {
        head = new Span;
        head->prev = head;
        head->next = head;
    }
    void Insert(Span *pos, Span *newSpan)
    {
        assert(pos);
        assert(newSpan);
        pos->prev->next = newSpan;
        newSpan->prev = pos->prev;
        newSpan->next = pos;
        pos->prev = newSpan;
    }
    void Erase(Span *delSpan)
    {
        assert(delSpan);
        assert(delSpan != head);
        delSpan->prev->next = delSpan->next;
        delSpan->next->prev = delSpan->prev;
    }
    ~SpanList()
    {
        delete head;
    }
private:
    Span *head;
    std::mutex mtx; //桶锁
};