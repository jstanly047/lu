#include <queue/SPSCQueue.h>
#include <thread>
#include <gtest/gtest.h>
#include <functional>
#include <iostream>
#include <chrono>

class SPSCQueueTest : public ::testing::Test
{
};

TEST_F(SPSCQueueTest, consumerProducerSequenceCheck)
{
    for (int numberExec = 0; numberExec < 15 ; numberExec++)
    {
        queue::SPSCQueue<2> testQueue{};
        unsigned int itemsCount = 1'000'000;
        auto producer = [&]()
        {
            for (unsigned int i = 0; i < itemsCount; i++)
            {
                testQueue.push_back(new unsigned int(i));
            }
        };

        auto consumer = [&]()
        {
            for (unsigned int expectedItem = 0; expectedItem < itemsCount; expectedItem++)
            {
                unsigned int *p = (unsigned int *)testQueue.pop();
                ASSERT_TRUE(expectedItem == *p);
                delete p;
            }
        };

        std::thread pt(producer);
        std::thread ct(consumer);
        pt.join();
        ct.join();
    }
}


TEST_F(SPSCQueueTest, checkWaitAndNotify)
{
    queue::SPSCQueue<2> testQueue{};
    std::atomic<unsigned int> readValue{}; 

    auto consumer = [&]()
    {
        for (unsigned int expectedItem = 1; expectedItem < 10; expectedItem++)
        {
            unsigned int *p = (unsigned int *)testQueue.pop();
            ASSERT_TRUE(expectedItem == *p);
            readValue = *p;
            delete p;
        }
    };

    std::thread ct(consumer);
    testQueue.push_back(new unsigned int(1));
    std::this_thread::sleep_for(std::chrono::microseconds(200));
    ASSERT_TRUE(readValue.load() == 1);
    testQueue.push_back(new unsigned int(2));
    std::this_thread::sleep_for(std::chrono::microseconds(200));
    ASSERT_TRUE(readValue.load() == 2);
    testQueue.push_back(new unsigned int(3));
    testQueue.push_back(new unsigned int(4));
    std::this_thread::sleep_for(std::chrono::microseconds(200));
    ASSERT_TRUE(readValue.load() == 4);
    testQueue.push_back(new unsigned int(5));
    testQueue.push_back(new unsigned int(6));
    testQueue.push_back(new unsigned int(7));
    testQueue.push_back(new unsigned int(8));
    testQueue.push_back(new unsigned int(9));
    ct.join();
    ASSERT_TRUE(readValue.load() == 9);
}