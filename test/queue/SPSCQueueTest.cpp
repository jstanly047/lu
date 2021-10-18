#include <queue/SPSCQueue.h>
#include <thread>
#include <gtest/gtest.h>
#include <functional>
#include <iostream>

class SPSCQueueTest : public ::testing::Test
{
public:
    SPSCQueueTest(){}
    ~SPSCQueueTest(){}
};

TEST_F(SPSCQueueTest, consumerProducerSequenceCheck){
    queue::SPSCQueue<2> testQueue{};
    constexpr unsigned int itemsCount = 200;
    auto producer = [&](){
        for (unsigned int i = 0 ; i < itemsCount; i++)
        {
            testQueue.push_back(new unsigned int(i));
        }
    };

    auto consumer = [&]() {
        for (unsigned int expectedItem = 0; expectedItem < itemsCount; expectedItem++)
        {
            unsigned int* p = (unsigned int*) testQueue.pop();
            unsigned int item = *p;
            delete p;
            ASSERT_TRUE(expectedItem == item);
        }
    };

    std::thread ct(consumer);
    std::thread pt(producer);
    pt.join();
    ct.join();
}