#include <iostream>
#include <vector>
#include <new>
#include "FixedMempool.hpp"
#include "Concurrent.hpp"
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
        for (int i = 1; i <= 5; ++i)
        {
            void *ptr = ConcurrentAlloc(6);
            auto output1 = std::this_thread::get_id();
            std::cout << std::format("{}:{}\n", output1, ptr);
        }
    }

    void AllocateFunc2()
    {
        for (int i = 1; i <= 514; ++i)
        {
            void *ptr = ConcurrentAlloc(25);
            auto output1 = std::this_thread::get_id();
            std::cout << std::format("{}:{}\n", output1, ptr);
        }
    }
    void AllocateFunc3()
    {
        for (int i = 1; i <=128; ++i)
        {
            void *ptr = ConcurrentAlloc(4096);
            auto output1 = std::this_thread::get_id();
            std::cout << std::format("{}:{}\n", output1, ptr);
        }
        for (int i = 1; i <= 514; ++i)
        {
            void *ptr = ConcurrentAlloc(25);
            auto output1 = std::this_thread::get_id();
            // std::cout << std::format("{}:{}\n", output1, ptr);
        }
        string *ptr2 = testAlloc<string>();
        cout << *ptr2 << endl;
    }
    void ConcurrentTest()
    {
        // AllocateFunc2();
        // thread t1(AllocateFunc1);
        // thread t2(AllocateFunc2);
        thread t3(AllocateFunc3);
        // t1.join();
        // t2.join();
        t3.join();
    }
};

int main()
{
    // TestObjectPool();
    ConcurrentTest::ConcurrentTest();
    return 0;
}