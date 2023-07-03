#include <queue/Queue.h>
#include <thread>
#include <gtest/gtest.h>
#include <functional>
#include <iostream>
#include <chrono>

constexpr unsigned int ITEMS_COUNT = 10'000'000;
constexpr unsigned int QUEUE_SIZE = 4000;

class TestQueue : public ::testing::Test
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

TEST_F(TestQueue, consumerProducerSequenceCheckForSPSC)
{
    using AtomicQueue = lu::queue::RetryDecorator<lu::queue::AtomicQueue2<unsigned int*, QUEUE_SIZE, true, true, false, true> >;
    AtomicQueue testQueue{};
    
    for (int numberExec = 0; numberExec < 2 ; numberExec++)
    {
        std::chrono::milliseconds produceTime;
        std::chrono::milliseconds consumerTime;
                
        auto producer = [&]()
        {
            auto started = std::chrono::high_resolution_clock::now();

            for (unsigned int i = 0; i < ITEMS_COUNT; i++)
            {
                testQueue.push(m_data[i]);
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

TEST_F(TestQueue, consumerSPSCNoMINIMIZE_CONTENTION)
{
    using AtomicQueue = lu::queue::RetryDecorator<lu::queue::AtomicQueue2<unsigned int*, QUEUE_SIZE, false, true, false, true> >;
    AtomicQueue testQueue{};
    
    for (int numberExec = 0; numberExec < 2 ; numberExec++)
    {
        std::chrono::milliseconds produceTime;
        std::chrono::milliseconds consumerTime;
                
        auto producer = [&]()
        {
            auto started = std::chrono::high_resolution_clock::now();

            for (unsigned int i = 0; i < ITEMS_COUNT; i++)
            {
                testQueue.push(m_data[i]);
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

TEST_F(TestQueue, consumerProducerSequenceCheck_SPSC_On_MPMC)
{
    using AtomicQueue = lu::queue::RetryDecorator<lu::queue::AtomicQueue2<unsigned int*, QUEUE_SIZE> >;
    AtomicQueue testQueue{};
    for (int numberExec = 0; numberExec < 2 ; numberExec++)
    {
        std::chrono::milliseconds produceTime;
        std::chrono::milliseconds consumerTime;
        
        auto producer = [&]()
        {
            auto started = std::chrono::high_resolution_clock::now();

            for (unsigned int i = 0; i < ITEMS_COUNT; i++)
            {
                testQueue.push(m_data[i]);
            }

            produceTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - started);
        };

        auto consumer = [&]()
        {
            auto started = std::chrono::high_resolution_clock::now();
            for (unsigned int expectedItem = 0; expectedItem < ITEMS_COUNT; expectedItem++)
            {
                testQueue.pop();
                // int *p = (unsigned int *)testQueue.pop();
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

TEST_F(TestQueue, consumerProducerCheckMPSC_On_MPSC)
{
    using AtomicQueue = lu::queue::RetryDecorator<lu::queue::AtomicQueue2<unsigned int*, QUEUE_SIZE> >;
    AtomicQueue testQueue{};
    for (int numberExec = 0; numberExec < 2 ; numberExec++)
    {
        std::chrono::high_resolution_clock::time_point producerStartTime[2];
        std::chrono::milliseconds produceTime[2];
        std::chrono::milliseconds consumerTime;
        
        
        auto producer = [&](int timeIndex)
        {
            producerStartTime[timeIndex] = std::chrono::high_resolution_clock::now();

            for (unsigned int i = 0; i < ITEMS_COUNT/2; i++)
            {
                testQueue.push(m_data[i]);
            }

            produceTime[timeIndex] = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - producerStartTime[timeIndex] );
        };

        auto consumer = [&]()
        {
            auto started = std::chrono::high_resolution_clock::now();
            for (unsigned int expectedItem = 0; expectedItem < ITEMS_COUNT ; expectedItem++)
            {
                testQueue.pop();
            }

            consumerTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - started);
        };

        std::thread pt1(producer, 0);
        std::thread pt2(producer, 1);
        std::thread ct(consumer);
        pt1.join();
        pt2.join();
        ct.join();
        std::cout << "Producer1 (ms):" << produceTime[0].count() <<std::endl;
        std::cout << "Producer2 (ms):" << produceTime[1].count() <<std::endl;
        std::cout << "Consumer (ms):"  << consumerTime.count() << std::endl;

    }
}

TEST_F(TestQueue, consumerProducerCheckMPSC_On_MPSCNoMINIMIZE_CONTENTION)
{
    using AtomicQueue = lu::queue::RetryDecorator<lu::queue::AtomicQueue2<unsigned int*, QUEUE_SIZE, false> >;
    AtomicQueue testQueue{};
    for (int numberExec = 0; numberExec < 2 ; numberExec++)
    {
        std::chrono::high_resolution_clock::time_point producerStartTime[2];
        std::chrono::milliseconds produceTime[2];
        std::chrono::milliseconds consumerTime;
        
        
        auto producer = [&](int timeIndex)
        {
            producerStartTime[timeIndex] = std::chrono::high_resolution_clock::now();

            for (unsigned int i = 0; i < ITEMS_COUNT/2; i++)
            {
                testQueue.push(m_data[i]);
            }

            produceTime[timeIndex] = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - producerStartTime[timeIndex] );
        };

        auto consumer = [&]()
        {
            auto started = std::chrono::high_resolution_clock::now();
            for (unsigned int expectedItem = 0; expectedItem < ITEMS_COUNT ; expectedItem++)
            {
                testQueue.pop();
            }

            consumerTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - started);
        };

        std::thread pt1(producer, 0);
        std::thread pt2(producer, 1);
        std::thread ct(consumer);
        pt1.join();
        pt2.join();
        ct.join();
        std::cout << "Producer1 (ms):" << produceTime[0].count() <<std::endl;
        std::cout << "Producer2 (ms):" << produceTime[1].count() <<std::endl;
        std::cout << "Consumer (ms):"  << consumerTime.count() << std::endl;

    }
}

TEST_F(TestQueue, consumerProducerCheckMPSC_On_MP$SC)
{
    using AtomicQueue = lu::queue::RetryDecorator<lu::queue::AtomicQueue2<unsigned int*, QUEUE_SIZE> >;
    AtomicQueue testQueue{};
    for (int numberExec = 0; numberExec < 2 ; numberExec++)
    {
        std::chrono::high_resolution_clock::time_point producerStartTime[4];
        std::chrono::milliseconds produceTime[4];
        std::chrono::milliseconds consumerTime;
        
        
        auto producer = [&](int timeIndex)
        {
            producerStartTime[timeIndex] = std::chrono::high_resolution_clock::now();

            for (unsigned int i = 0; i < ITEMS_COUNT/4; i++)
            {
                testQueue.push(m_data[i]);
            }

            produceTime[timeIndex] = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - producerStartTime[timeIndex] );
        };

        auto consumer = [&]()
        {
            auto started = std::chrono::high_resolution_clock::now();
            for (unsigned int expectedItem = 0; expectedItem < ITEMS_COUNT ; expectedItem++)
            {
                testQueue.pop();
            }

            consumerTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - started);
        };

        std::thread pt1(producer, 0);
        std::thread pt2(producer, 1);
        std::thread pt3(producer, 2);
        std::thread pt4(producer, 3);
        std::thread ct(consumer);
        pt1.join();
        pt2.join();
        pt3.join();
        pt4.join();
        ct.join();
        std::cout << "Producer1 (ms):" << produceTime[0].count() <<std::endl;
        std::cout << "Producer2 (ms):" << produceTime[1].count() <<std::endl;
        std::cout << "Producer3 (ms):" << produceTime[2].count() <<std::endl;
        std::cout << "Producer4 (ms):" << produceTime[3].count() <<std::endl;
        std::cout << "Consumer (ms):"  << consumerTime.count() << std::endl;

    }
}

TEST_F(TestQueue, consumerProducerCheckMPMC)
{
    using AtomicQueue = lu::queue::RetryDecorator<lu::queue::AtomicQueue2<unsigned int*, QUEUE_SIZE> >;
    AtomicQueue testQueue{};
    for (int numberExec = 0; numberExec < 2 ; numberExec++)
    {
        std::chrono::high_resolution_clock::time_point producerStartTime[2];
        std::chrono::milliseconds produceTime[2];
        std::chrono::high_resolution_clock::time_point consumerStartTime[2];
        std::chrono::milliseconds consumerTime[2];
        
        auto producer = [&](int timeIndex)
        {
            producerStartTime[timeIndex] = std::chrono::high_resolution_clock::now();

            for (unsigned int i = 0; i < ITEMS_COUNT/2; i++)
            {
                testQueue.push(m_data[i]);
            }

            produceTime[timeIndex] = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - producerStartTime[timeIndex] );
        };

        auto consumer = [&](int timeIndex)
        {
            consumerStartTime[timeIndex] = std::chrono::high_resolution_clock::now();
            for (unsigned int expectedItem = 0; expectedItem < ITEMS_COUNT/2; expectedItem++)
            {
                testQueue.pop();
            }

            consumerTime[timeIndex] = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - consumerStartTime[timeIndex] );
        };

        std::thread pt1(producer, 0);
        std::thread pt2(producer, 1);
        std::thread ct1(consumer, 0);
        std::thread ct2(consumer, 1);
        pt1.join();
        pt2.join();
        ct1.join();
        ct2.join();
        std::cout << "Producer1 (ms):" << produceTime[0].count() <<std::endl;
        std::cout << "Producer2 (ms):" << produceTime[1].count() <<std::endl;
        std::cout << "Consumer1 (ms):"  << consumerTime[0].count() << std::endl;
        std::cout << "Consumer2 (ms):"  << consumerTime[1].count() << std::endl;

    }
}


