#include <platform/thread/ClientThread.h>
#include <platform/thread/ConnectionThread.h>
#include <platform/thread/MockWorkerThread.h>
#include <platform/thread/ServerThread.h>
#include <platform/thread/WorkerThread.h>
#include <platform/socket/DataSocket.h>
#include <platform/socket/data_handler/String.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>

#include <thread>
#include <condition_variable>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace lu::platform::thread;

namespace
{
    SeverConfig getServerConfig()
    {
        SeverConfig serverConfig{};
        serverConfig.TIMER_IN_MSEC = 100u;
        serverConfig.CREATE_NEW_THREAD = true;
        serverConfig.NUMBER_OF_CLIENT_HANDLE_THREADS = 1u;
        return serverConfig;
    }

    struct ReplyMsg
    {
        channel::ChannelID channelID;
        unsigned int intValue;
    };

    lu::utils::WaitForCount waitForConnectionThread(1u);
    lu::utils::WaitForCount waitForMessage(1u);

    class ConnectionThreadCallback
    {
    public:
        using StringDataSocket=lu::platform::socket::DataSocket<ConnectionThreadCallback, lu::platform::socket::data_handler::String>;
        bool onInit() { return true; }
        void onStart() {}
        void onAppMsg(void* msg) 
        {
            auto value = reinterpret_cast<int*>(msg);
            EXPECT_EQ(100, *value);
            delete value;
            waitForMessage.increment();
        }

        // EXPECT_CALL(connectionThreadCallback,  onStartComplete());
        void onConnection([[maybe_unused]]StringDataSocket &dataSocket)
        {
        }

        void onData([[maybe_unused]]StringDataSocket &dataSocket,[[maybe_unused]] void *message)
        {
        }

        void onClientClose([[maybe_unused]]StringDataSocket &dataSocket)
        {
        }

        void onExit() 
        {
        }

        void onTimer(const lu::platform::FDTimer<ConnectionThreadCallback> &) 
        {
            if (m_isFirstTimer == false)
            {
                waitForConnectionThread.increment();
                m_isFirstTimer = false;
            }
        }

    private:
        bool m_isFirstTimer = false;
    };
}

class TestConnectionThread : public ::testing::Test
{
public:
    TestConnectionThread() :
                    connectionThread("Client",connectionThreadCallback, EventThreadConfig(getServerConfig())),
                    workerConsumer("TestConsumer", mockConsumerCallback)
    {
        connectionThread.connectTo("localhost", "10000");
    }
protected:
    void SetUp() override 
    {
        connectionThread.connectFrom(workerConsumer);
        EXPECT_CALL(mockConsumerCallback,  onInit()).WillOnce(::testing::Return(true));
        workerConsumer.init();
    }

    void TearDown() override 
    {
        connectionThread.stop();
        workerConsumer.stop();
        connectionThread.join();
        workerConsumer.join();
    }


    ConnectionThreadCallback connectionThreadCallback;
    ConnectionThread<ConnectionThreadCallback, ConnectionThreadCallback::StringDataSocket> connectionThread;

    MockWorkerThread mockConsumerCallback;
    WorkerThread<MockWorkerThread> workerConsumer;
};

TEST_F(TestConnectionThread, TestEventChannel)
{
    EXPECT_CALL(mockConsumerCallback,  onStart()).WillOnce(::testing::Invoke(
        [&]()
        {
            waitForConnectionThread.wait();
            lu::platform::thread::LuThread::transferMsgToServerThread(connectionThread.getChannelID(), new int(100));
        }));

    EXPECT_CALL(mockConsumerCallback,  onExit()).Times(1);
    EXPECT_CALL(mockConsumerCallback,  onMsg(::testing::_)).Times(0);

    connectionThread.init();
    connectionThread.start(true);
    workerConsumer.start();
    waitForMessage.wait();
}