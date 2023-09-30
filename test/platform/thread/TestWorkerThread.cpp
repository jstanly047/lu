
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

            if (stopCount == 6u)
            {
                m_workerProducer.stop();
                return;
            }

            stopCount++;
            auto data = reinterpret_cast<unsigned int*>(channelData.data);
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

    auto consumerIndx =  m_workerProducer.getThreadIndex(m_workerConsumer.getName());
    auto producerIndx =  m_workerConsumer.getThreadIndex(m_workerProducer.getName());

    EXPECT_CALL(m_mockProducerCallback,  onInit()).WillOnce(::testing::Return(true));
    EXPECT_CALL(m_mockProducerCallback,  onStart()).WillOnce(::testing::Invoke(
        [&]()
        {
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

            if (stopCount == 6u)
            {
                m_workerProducer.stop();
                return;
            }

            stopCount++;
            auto data = reinterpret_cast<unsigned int*>(channelData.data);
            EXPECT_EQ(producerExpectedValue, *data);
            m_workerProducer.transferMsg(consumerIndx, new int(++count));
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