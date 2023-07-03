#include <queue/SPSCQueue.h>
#include <thread>
#include <gtest/gtest.h>
#include <functional>
#include <iostream>
#include <chrono>
constexpr unsigned int ITEMS_COUNT = 10'000'000;

class TestSPSCQueue : public ::testing::Test
{
protected:
    void SetUp() override 
    {
        m_data.reserve(ITEMS_COUNT);

        for (unsigned int i = 0; i < ITEMS_COUNT; i++)
        {
            m_data.push_back(new unsigned int(i));
        }
    }

    void TearDown() override 
    {
        for (unsigned int i = 0; i < ITEMS_COUNT; i++)
        {
            delete m_data[i];
        }   
    }

    std::vector <unsigned int*> m_data;
};

TEST_F(TestSPSCQueue, consumerProducerSequenceCheck)
{
    lu::queue::SPSCQueue<2> testQueue{};
    for (int numberExec = 0; numberExec < 2 ; numberExec++)
    {
        std::chrono::milliseconds produceTime;
        std::chrono::milliseconds consumerTime;
        
        auto producer = [&]()
        {
            auto started = std::chrono::high_resolution_clock::now();

            for (unsigned int i = 0; i < ITEMS_COUNT; i++)
            {
                testQueue.push_back(m_data[i]);
            }

            produceTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - started);
        };

        auto consumer = [&]()
        {
            auto started = std::chrono::high_resolution_clock::now();
            for (unsigned int expectedItem = 0; expectedItem < ITEMS_COUNT; expectedItem++)
            {
                testQueue.pop();
                //unsigned int *p = (unsigned int *)testQueue.pop();
                //ASSERT_TRUE(expectedItem == *p);
            }

            consumerTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - started);
        };

        std::thread pt(producer);
        std::thread ct(consumer);
        pt.join();
        ct.join();
        std::cout << "Producer (ms):" << produceTime.count() <<std::endl;
        std::cout << "Consumer (ms):"  << consumerTime.count() << std::endl;

    }
}


TEST_F(TestSPSCQueue, checkWaitAndNotify)
{
    lu::queue::SPSCQueue<2> testQueue{};
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