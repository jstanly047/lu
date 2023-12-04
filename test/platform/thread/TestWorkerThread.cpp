
#include <platform/thread/WorkerThread.h>
#include <platform/thread/MockWorkerThread.h>
#include <condition_variable>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace lu::platform::thread;

class TestWorkerThread : public ::testing::Test
{
public:
    TestWorkerThread(): m_workerConsumer("TestConsumer", m_mockConsumerCallback),
                        m_workerProducer("TestProducer", m_mockProducerCallback)
    {
    }

protected:
    void SetUp() override
    {

    }

    void TearDown() override 
    {

    }
    MockWorkerThread m_mockConsumerCallback;
    MockWorkerThread m_mockProducerCallback;
    WorkerThread<MockWorkerThread> m_workerConsumer;
    WorkerThread<MockWorkerThread> m_workerProducer;
};

TEST_F(TestWorkerThread, TestMessageTransferByTreadName)
{
    unsigned int count = 0;
    unsigned int consumerExpectedValue = 1;
    unsigned int producerExpectedValue = 2;

    EXPECT_CALL(m_mockProducerCallback,  onInit()).WillOnce(::testing::Return(true));
    EXPECT_CALL(m_mockProducerCallback,  onStart()).WillOnce(::testing::Invoke(
        [&]()
        {
            m_workerProducer.transferMsg(m_workerConsumer.getName(), new int(++count));
        }));
    EXPECT_CALL(m_mockProducerCallback,  onMsg(::testing::_)).WillRepeatedly(::testing::Invoke(
        [&](channel::ChannelData channelData)
        {
            if (channelData.data == nullptr)
            {
                EXPECT_EQ(channelData.channelID, m_workerProducer.getChannelID());
                return;
            }

            static unsigned stopCount = 1;
            EXPECT_EQ(channelData.channelID, m_workerConsumer.getChannelID());
            auto data = reinterpret_cast<unsigned int*>(channelData.data);

            if (stopCount == 6u)
            {
                m_workerProducer.stop();
                delete data;
                return;
            }

            stopCount++;
            EXPECT_EQ(producerExpectedValue, *data);
            m_workerProducer.transferMsg(m_workerConsumer.getName(), new int(++count));
            producerExpectedValue += 2;
            delete data;
        }));
    EXPECT_CALL(m_mockProducerCallback,  onExit()).Times(1);
    
    EXPECT_CALL(m_mockConsumerCallback,  onInit()).WillOnce(::testing::Return(true));
    EXPECT_CALL(m_mockConsumerCallback,  onStart());
    EXPECT_CALL(m_mockConsumerCallback,  onMsg(::testing::_)).WillRepeatedly(::testing::Invoke(
        [&](channel::ChannelData channelData)
        {
            if (channelData.data == nullptr)
            {
                EXPECT_EQ(channelData.channelID, m_workerConsumer.getChannelID());
                return;
            }

            auto data = reinterpret_cast<unsigned int*>(channelData.data);
            EXPECT_EQ(channelData.channelID, m_workerProducer.getChannelID());
            EXPECT_EQ(consumerExpectedValue, *data);
            m_workerConsumer.transferMsg(m_workerProducer.getName(), new int(*data  * 2));
            consumerExpectedValue++;
            delete data;

            if (consumerExpectedValue == 7u)
            {
                m_workerConsumer.stop();
            }
        }));
    EXPECT_CALL(m_mockConsumerCallback,  onExit()).Times(1);

    m_workerProducer.connect(m_workerConsumer);
    m_workerConsumer.connect(m_workerProducer);
    m_workerConsumer.init();
    m_workerProducer.init();
    m_workerConsumer.start();
    m_workerProducer.start();
    m_workerProducer.join();
    m_workerConsumer.join();
}

TEST_F(TestWorkerThread, TestMessageTransferByTreadIndex)
{
    unsigned int count = 0;
    unsigned int consumerExpectedValue = 1;
    unsigned int producerExpectedValue = 2;
    m_workerProducer.connect(m_workerConsumer);
    m_workerConsumer.connect(m_workerProducer);

    unsigned int consumerIndx = std::numeric_limits<unsigned int>::max();;
     
    unsigned int producerIndx = std::numeric_limits<unsigned int>::max();;

    EXPECT_CALL(m_mockProducerCallback,  onInit()).WillOnce(::testing::Return(true));
    EXPECT_CALL(m_mockProducerCallback,  onStart()).WillOnce(::testing::Invoke(
        [&]()
        {
            consumerIndx = LuThread::getThreadIndex(m_workerConsumer.getName());
            m_workerProducer.transferMsg(consumerIndx, new int(++count));
        }));
    EXPECT_CALL(m_mockProducerCallback,  onMsg(::testing::_)).WillRepeatedly(::testing::Invoke(
        [&](channel::ChannelData channelData)
        {
            if (channelData.data == nullptr)
            {
                EXPECT_EQ(channelData.channelID, m_workerProducer.getChannelID());
                return;
            }

            static unsigned stopCount = 1;
            EXPECT_EQ(channelData.channelID, m_workerConsumer.getChannelID());
            auto data = reinterpret_cast<unsigned int*>(channelData.data);

            if (stopCount == 6u)
            {
                m_workerProducer.stop();
                delete data;
                return;
            }

            stopCount++;
            EXPECT_EQ(producerExpectedValue, *data);
            m_workerProducer.transferMsg(consumerIndx, new int(++count));
            producerExpectedValue += 2;
            delete data;
        }));
    EXPECT_CALL(m_mockProducerCallback,  onExit()).Times(1);
    
    EXPECT_CALL(m_mockConsumerCallback,  onInit()).WillOnce(::testing::Return(true));
    EXPECT_CALL(m_mockConsumerCallback,  onStart()).WillOnce(::testing::Invoke(
        [&]()
        {
            producerIndx = m_workerConsumer.getThreadIndex(m_workerProducer.getName());
        }));

    EXPECT_CALL(m_mockConsumerCallback,  onMsg(::testing::_)).WillRepeatedly(::testing::Invoke(
        [&](channel::ChannelData channelData)
        {
            if (channelData.data == nullptr)
            {
                EXPECT_EQ(channelData.channelID, m_workerConsumer.getChannelID());
                return;
            }

            auto data = reinterpret_cast<unsigned int*>(channelData.data);
            EXPECT_EQ(channelData.channelID, m_workerProducer.getChannelID());
            EXPECT_EQ(consumerExpectedValue, *data);
            m_workerConsumer.transferMsg(producerIndx, new int(*data  * 2));
            consumerExpectedValue++;
            delete data;

            if (consumerExpectedValue == 7u)
            {
                m_workerConsumer.stop();
            }
        }));
    EXPECT_CALL(m_mockConsumerCallback,  onExit()).Times(1);

    
    m_workerConsumer.init();
    m_workerProducer.init();
    m_workerConsumer.start();
    m_workerProducer.start();
    m_workerProducer.join();
    m_workerConsumer.join();
}

TEST_F(TestWorkerThread, multipleProducerAndOneConsumer)
{

    MockWorkerThread m_mockConsumerCallbackSecond;
    WorkerThread<MockWorkerThread> m_workerConsumerSecond("TestConsumerSecond", m_mockConsumerCallbackSecond);
    
    unsigned int consumerExpectedValue = 1;
    unsigned int consumerSecondExpectedValue = 2;
    m_workerProducer.connect(m_workerConsumer);
    m_workerProducer.connect(m_workerConsumerSecond);
    m_workerConsumer.connect(m_workerProducer);
    m_workerConsumerSecond.connect(m_workerProducer);

    auto consumerIndx = std::numeric_limits<unsigned int>::max();
    auto producerIndx = std::numeric_limits<unsigned int>::max();
    auto consumerSecondIndx = std::numeric_limits<unsigned int>::max();  
    auto producerSecondIndx = std::numeric_limits<unsigned int>::max();

    EXPECT_CALL(m_mockProducerCallback,  onInit()).WillOnce(::testing::Return(true));
    EXPECT_CALL(m_mockProducerCallback,  onStart()).WillOnce(::testing::Invoke(
        [&]()
        {
            for (unsigned int count= 1; count < 7u; count++)
            {
                if (count%2 == 0)
                {
                    consumerSecondIndx = m_workerProducer.getThreadIndex(m_workerConsumerSecond.getName());
                    m_workerProducer.transferMsg(consumerSecondIndx, new int(count));
                }
                else
                {
                    consumerIndx = m_workerProducer.getThreadIndex(m_workerConsumer.getName());
                    m_workerProducer.transferMsg(consumerIndx, new int(count));
                }
            }

           
        }));
    EXPECT_CALL(m_mockProducerCallback,  onMsg(::testing::_)).WillRepeatedly(::testing::Invoke(
        [&](channel::ChannelData channelData)
        {
            static unsigned int countMsg = 0;
            static std::set<unsigned int> dataCache;
            if (channelData.data == nullptr)
            {
                EXPECT_EQ(channelData.channelID, m_workerProducer.getChannelID());
                EXPECT_THAT(dataCache, ::testing::ElementsAre(2u, 4u, 6u, 8u, 10u, 12u));
                return;
            }

            auto data = reinterpret_cast<unsigned int*>(channelData.data);
            dataCache.insert(*data);

            if ((*data/2) % 2 == 0)
            {
                EXPECT_EQ(channelData.channelID, m_workerConsumerSecond.getChannelID());
            }
            else
            {
                EXPECT_EQ(channelData.channelID, m_workerConsumer.getChannelID());
            }

            delete data;
            countMsg++;
            if (countMsg == 6u)
            {
                m_workerProducer.stop();
            }
        }));
    EXPECT_CALL(m_mockProducerCallback,  onExit()).Times(1);
    
    EXPECT_CALL(m_mockConsumerCallback,  onInit()).WillOnce(::testing::Return(true));
    EXPECT_CALL(m_mockConsumerCallback,  onStart()).WillOnce(::testing::Invoke(
        [&]()
        {
            producerIndx = m_workerConsumer.getThreadIndex(m_workerProducer.getName());
        }));
    EXPECT_CALL(m_mockConsumerCallback,  onMsg(::testing::_)).WillRepeatedly(::testing::Invoke(
        [&](channel::ChannelData channelData)
        {
            if (channelData.data == nullptr)
            {
                EXPECT_EQ(channelData.channelID, m_workerConsumer.getChannelID());
                return;
            }

            auto data = reinterpret_cast<unsigned int*>(channelData.data);
            EXPECT_EQ(channelData.channelID, m_workerProducer.getChannelID());
            EXPECT_EQ(consumerExpectedValue, *data);
            m_workerConsumer.transferMsg(producerIndx, new int(*data  * 2));
            consumerExpectedValue += 2;
            delete data;

            if (consumerExpectedValue == 7u)
            {
                m_workerConsumer.stop();
            }
        }));
    EXPECT_CALL(m_mockConsumerCallback,  onExit()).Times(1);

    EXPECT_CALL(m_mockConsumerCallbackSecond,  onInit()).WillOnce(::testing::Return(true));
    EXPECT_CALL(m_mockConsumerCallbackSecond,  onStart()).WillOnce(::testing::Invoke(
        [&]()
        {
            producerSecondIndx = m_workerConsumerSecond.getThreadIndex(m_workerProducer.getName());
        }));

    EXPECT_CALL(m_mockConsumerCallbackSecond,  onMsg(::testing::_)).WillRepeatedly(::testing::Invoke(
        [&](channel::ChannelData channelData)
        {
            if (channelData.data == nullptr)
            {
                EXPECT_EQ(channelData.channelID, m_workerConsumerSecond.getChannelID());
                return;
            }

            auto data = reinterpret_cast<unsigned int*>(channelData.data);
            EXPECT_EQ(channelData.channelID, m_workerProducer.getChannelID());
            EXPECT_EQ(consumerSecondExpectedValue, *data);
            m_workerConsumerSecond.transferMsg(producerSecondIndx, new int(*data  * 2));
            consumerSecondExpectedValue += 2;
            delete data;

            if (consumerSecondExpectedValue == 8u)
            {
                m_workerConsumerSecond.stop();
            }
        }));
    EXPECT_CALL(m_mockConsumerCallbackSecond,  onExit()).Times(1);

    
    m_workerConsumer.init();
    m_workerConsumerSecond.init();
    m_workerProducer.init();
    m_workerConsumer.start();
    m_workerConsumerSecond.start();
    m_workerProducer.start();
    m_workerProducer.join();
    m_workerConsumer.join();
    m_workerConsumerSecond.join();
}