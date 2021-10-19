#include <queue/SPSCQueue.h>
#include <thread>
#include <gtest/gtest.h>
#include <functional>
#include <iostream>
#include <chrono>

class SPSCQueueTest : public ::testing::Test
{
public:
    SPSCQueueTest(){}
    ~SPSCQueueTest(){}
};

TEST_F(SPSCQueueTest, queueIsInitializedToNull)
{
    struct Queue{
            std::atomic<void*> m_buffer[100]{nullptr};
            Queue* next{nullptr};
    };
    
    Queue* queue = new Queue();

    for (int i = 0; i < 100;i++)
    {
        ASSERT_TRUE(queue->m_buffer[i] == nullptr);
    }

    ASSERT_TRUE(queue->next == nullptr);

}


TEST_F(SPSCQueueTest, consumerProducerSequenceCheck)
{
    //long long int totalCTimeMs = 0;
    //long long int totalPTimeMs = 0;
    for (int numberExec = 0; numberExec < 15 ; numberExec++)
    {
        queue::SPSCQueue<2> testQueue{};
        unsigned int itemsCount = 10'000'000;
        auto producer = [&]()
        {
            //auto start = std::chrono::high_resolution_clock::now();
            for (unsigned int i = 0; i < itemsCount; i++)
            {
                testQueue.push_back(new unsigned int(i));
            }

            //auto end = std::chrono::high_resolution_clock::now();
            //totalPTimeMs += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        };

        auto consumer = [&]()
        {
            //auto start = std::chrono::high_resolution_clock::now();
            for (unsigned int expectedItem = 0; expectedItem < itemsCount; expectedItem++)
            {
                unsigned int *p = (unsigned int *)testQueue.pop();
                ASSERT_TRUE(expectedItem == *p);
                delete p;
            }
            //auto end = std::chrono::high_resolution_clock::now();
            //totalCTimeMs += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        };

        std::thread pt(producer);
        std::thread ct(consumer);
        pt.join();
        ct.join();
    }
    
    //std::cout << totalCTimeMs + totalCTimeMs << std::endl;
}