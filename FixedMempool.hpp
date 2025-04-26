#pragma once

#include <iostream>
#include <vector>
#include <Windows.h>
#include <memoryapi.h>
using std::cin;
using std::cout;



template <class T>
class FLMemPool
{
public:
    T *New()
    {
        T *ret = nullptr;
        if (freelist)
        {
            // 如果回收站有空间，直接将内存块从回收站中取出使用
            ret = (T *)freelist;
            freelist = *(void **)freelist;
        }
        else
        {
            if (leftMemLen < sizeof(T))
            {
                leftMemLen = 1024 * 128;
                memory = (char *)VirtualAlloc(nullptr, 1024 * 128, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
                if (!memory)
                {
                    throw std::bad_alloc();
                }
                // 析构时释放
                blocks.push_back(memory);
            }
            ret = (T *)memory;
            // 保证可以存下单个地址长度以便建立链表
            size_t obj_len = sizeof(T) > sizeof(void *) ? sizeof(T) : sizeof(void *);
            memory += obj_len;
            leftMemLen -= obj_len;
        }
        // 定位new调用构造函数
        new (ret) T;
        return ret;
    }

    void Delete(T *obj)
    {
        obj->~T();
        // 头插法将freelist的地址存放在obj首个地址长度当中
        *(void **)obj = freelist;
        freelist = obj;
    }

    // 最终释放内存
    ~FLMemPool()
    {
        freelist = nullptr;
        for (char *block : blocks)
            VirtualFree(block, 0, MEM_RELEASE);
    }

private:
    char *memory = nullptr;
    size_t leftMemLen = 0;
    void *freelist = nullptr;
    std::vector<char *> blocks;
};