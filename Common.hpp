#pragma once

#include <iostream>
#include <vector>
#include <Windows.h>
#include <memoryapi.h>
#include <assert.h>
#include <thread>
#include <mutex>
#include <algorithm>
using std::cin;
using std::cout;

#undef min
#undef max

//使得页编号适配不同的windows环境
// #ifdef _WIN64
//     using PAGE_ID = unsigned long long;
// #elif _WIN32
//     using PAGE_ID = size_t;
// #endif

//size_t天然就适配不同环境
using PAGE_ID = size_t;

const inline int MAX_BYTES = 256 * 1024;
const inline int NFREELIST = 208;
const inline int PAGE_SHIFT = 13;
const inline size_t low_level = 2;
const inline size_t up_level = 512;
const inline size_t middle_level = (low_level + up_level) >> 1;
const inline size_t NPAGES = 128;


// inline void* &NextObj(void *obj)
// {
//     return *(void**)obj;
// }

union Object {
    Object *next = nullptr;
    std::byte data[0];
};

class FreeList
{
public:
    void Push(Object *obj)
    {
        assert(obj);
        obj->next = _freelist;
        // NextObj(obj) = _freelist;
        _freelist = obj;
        
    }
    Object *Pop()
    {
        assert(_freelist);
        Object *obj = _freelist;
        _freelist = obj->next;
        return obj;
    }
    void PushRange(Object *start, Object *end)
    {
        end->next = _freelist;
        _freelist = start;
    }
    bool Empty()
    {
        return _freelist == nullptr;
    }
    size_t& MaxSize()
    {
        return _maxSize;
    }
private:
    Object *_freelist = nullptr;
    size_t _maxSize = 1;
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
    static inline size_t NumMoveSize(size_t byte)
    {
        //控制获取的freelist在[2, 512]范围内
        assert(byte > 0);
        size_t num = MAX_BYTES / byte;
        if(num < low_level) num = low_level;
        else if(num > up_level) num = up_level;
        return num;
    }
    static inline size_t NumMovePage(size_t byte)
    {
        size_t num = NumMoveSize(byte);
        size_t total_byte = num * byte;
        size_t page_num = total_byte >> PAGE_SHIFT;
        //page_num至少为1
        if(page_num == 0) return 1;
        return page_num;
    }
};

struct Span
{
    PAGE_ID pageId; //页编号
    Span *prev = nullptr;
    Span *next = nullptr;
    size_t n = 0; //页数量
    size_t useCount = 0;
    Object *freelist = nullptr;
};

class SpanList
{
public:
    Span *Begin()
    {
        return head->next;
    }
    Span *End()
    {
        return head;
    }
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
    void PushFront(Span *newSpan)
    {
        Insert(Begin(), newSpan);
    }
    Span *PopFront()
    {
        Span *front = head->next;
        Erase(front);
        return front;
    }
    bool Empty()
    {
        return head->next == head;
    }
    ~SpanList()
    {
        delete head;
    }
private:
    Span *head;
public:
    std::mutex mtx; //桶锁
};