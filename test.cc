#include <iostream>
#include <vector>
#include <new>
#include "FixedMempool.hpp"
#include "Concurrent.hpp"
#include "PageCache.hpp"
#include "CentralCache.hpp"
using namespace std;

namespace FixedMempoolTestTunc
{
    struct TreeNode
    {
        int _val;
        TreeNode *_left;
        TreeNode *_right;
        TreeNode()
            : _val(0), _left(nullptr), _right(nullptr)
        {
        }
    };
    void TestObjectPool()
    {
        // 申请释放的轮次
        const size_t Rounds = 3;
        // 每轮申请释放多少次
        const size_t N = 1000000;
        size_t begin1 = clock();
        std::vector<TreeNode *> v1;
        v1.reserve(N);
        for (size_t j = 0; j < Rounds; ++j)
        {
            for (int i = 0; i < N; ++i)
            {
                v1.push_back(new TreeNode);
            }
            for (int i = 0; i < N; ++i)
            {
                delete v1[i];
            }
            v1.clear();
        }
        size_t end1 = clock();
        FLMemPool<TreeNode> TNPool;
        size_t begin2 = clock();
        std::vector<TreeNode *> v2;
        v2.reserve(N);
        for (size_t j = 0; j < Rounds; ++j)
        {
            for (int i = 0; i < N; ++i)
            {
                v2.push_back(TNPool.New());
            }
            for (int i = 0; i < N; ++i)
            {
                TNPool.Delete(v2[i]);
            }
            v2.clear();
        }
        size_t end2 = clock();
        cout << "new cost time:" << end1 - begin1 << endl;
        cout << "object pool cost time:" << end2 - begin2 << endl;
    }
};

namespace ConcurrentTest
{
    void AllocateFunc1()
    {
        vector<void *> tmp;
        for (int i = 1; i <= 1024; ++i)
        {
            void *ptr = ConcurrentAlloc(6);
            tmp.push_back(ptr);
            auto output1 = std::this_thread::get_id();
            // std::cout << std::format("{}:{}\n", output1, ptr);
        }
        for(auto e: tmp) ConcurrentFree(e);
    }

    void AllocateFunc2()
    {
        vector<void *> tmp;
        for (int i = 1; i <= 24; ++i)
        {
            void *ptr = ConcurrentAlloc(32);
            tmp.push_back(ptr);
            auto output1 = std::this_thread::get_id();
            // std::cout << std::format("{}:{}\n", output1, ptr);
        }
        // cout << std::format("{}", timer.GetCurrentDuration()) << endl;
        // timer.Start();
        // for(auto e: tmp) ConcurrentFree(e);
        // cout << std::format("{}", timer.GetCurrentDuration()) << endl;
        // timer.End();
        // cout << CentralCache::GetInstance()->GetMutexTime() << endl;
        // cout << PageCache::GetInstance()->mtx.GetTime() << endl;
    }
    void CPPAllocateFunc2()
    {
        vector<void *> tmp;
        for (int i = 1; i <= 24 * 1024; ++i)
        {
            void *ptr = malloc(32);
            tmp.push_back(ptr);
            auto output1 = std::this_thread::get_id();
            // std::cout << std::format("{}:{}\n", output1, ptr);
        }
        for(auto e: tmp) free(e);
    }
    void AllocateFunc3()
    {
        vector<void *> tmp;
        for (int i = 1; i <=512; ++i)
        {
            void *ptr = ConcurrentAlloc(4096);
            tmp.push_back(ptr);
            auto output1 = std::this_thread::get_id();
            // std::cout << std::format("{}:{}\n", output1, ptr);
        }
        for(auto e: tmp) ConcurrentFree(e);
        
    }
    void AllocateFunc4()
    {
        vector<void *> tmp;
        for (int i = 1; i <=4; ++i)
        {
            void *ptr = ConcurrentAlloc(257 * 1024);
            tmp.push_back(ptr);
            auto output1 = std::this_thread::get_id();
            // std::cout << std::format("{}:{}\n", output1, ptr);
        }
        for(auto e: tmp) ConcurrentFree(e);
    }
    void AllocateFunc5()
    {
        vector<void *> tmp;
        for (int i = 1; i <=4; ++i)
        {
            void *ptr = ConcurrentAlloc(512 * 1024);
            tmp.push_back(ptr);
            auto output1 = std::this_thread::get_id();
            // std::cout << std::format("{}:{}\n", output1, ptr);
        }
        for(auto e: tmp) ConcurrentFree(e);
    }
    void AllocateFunc6()
    {
        vector<void *> tmp;
        for (int i = 1; i <=6; ++i)
        {
            void *ptr = ConcurrentAlloc(129 * 8 * 1024);
            tmp.push_back(ptr);
            auto output1 = std::this_thread::get_id();
            // std::cout << std::format("{}:{}\n", output1, ptr);
        }
        for(auto e: tmp) ConcurrentFree(e);
    }
    void ConcurrentTest()
    {
        // AllocateFunc3();
        // thread t1(AllocateFunc1);
        // thread t2(AllocateFunc2);
        // thread t3(AllocateFunc3);
        // t1.join();
        // t2.join();
        // t3.join();
        AllocateFunc2();
        // for(int i = 1; i <= 128; ++i)
        // {
        //     if(PageCache::GetInstance()->_spanLists[i].Begin() != \
        //     PageCache::GetInstance()->_spanLists[i].End())
        //     {
        //         cout << i << ":";
        //     int num = 0;
        //     for(auto it = PageCache::GetInstance()->_spanLists[i].Begin(); \
        //     it != PageCache::GetInstance()->_spanLists[i].End(); it = it->next)
        //     ++num; cout << num << endl;
        //     }
        // }
    }
    void MaxSizeTest()
    {
        thread t1(AllocateFunc4);
        thread t2(AllocateFunc5);
        thread t3(AllocateFunc6);
        t1.join();
        t2.join();
        t3.join();
    }
};

int main()
{
    // TestObjectPool();
    // while(true)
    // {ConcurrentTest::ConcurrentTest();ConcurrentTest::ConcurrentTest();}
    ConcurrentTest::ConcurrentTest();
    return 0;
}