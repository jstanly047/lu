#include <platform/thread/MockConnectionThreadCallback.h>
#include <platform/thread/ConnectionThread.h>
#include <platform/thread/ServerSingleThread.h>
#include <platform/thread/MockWorkerThread.h>
#include <platform/thread/WorkerThread.h>


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
        return serverConfig;
    }

    class ServerSingleThreadCallback
    {
    public:
        using StringDataSocket = lu::platform::socket::DataSocket<ServerSingleThreadCallback, lu::platform::socket::data_handler::String>;
        ServerSingleThreadCallback() {}

        bool onInit() { return true; }
        void onStart() {}
        void onExit() {}

        void onNewConnection([[maybe_unused]] StringDataSocket &dataSocket)
        {
        }

        void onAppMsg(void *msg, [[maybe_unused]] lu::platform::thread::channel::ChannelID channelID)
        {
            auto replyMsg = reinterpret_cast<lu::platform::socket::data_handler::String::Message*>(msg);
            m_dataSocket->sendMsg(replyMsg, sizeof(lu::platform::socket::data_handler::String::Message));
            delete replyMsg;
        }

        void onTimer(const lu::platform::FDTimer<ServerSingleThreadCallback> &)
        {
        }

        void onClientClose([[maybe_unused]] StringDataSocket &dataSocket)
        {
        }

        void onData(StringDataSocket &dataSocket, void *message)
        {
            auto *strMessage = reinterpret_cast<lu::platform::socket::data_handler::String::Message *>(message);

            if (strMessage->getString() == "Ping")
            {
                lu::platform::socket::data_handler::String::Message reply("Pong");
                dataSocket.sendMsg(&reply, sizeof(lu::platform::socket::data_handler::String::Message));
                delete strMessage;
            }
            else if (strMessage->getString() == "GetName")
            {
                m_dataSocket = &dataSocket;
                LuThread::transferMsg(m_workerThread->getName(), strMessage);
            }
        }

        void setWorkerThread(WorkerThread<MockWorkerThread>& workerThread)
        {
            m_workerThread = &workerThread;
        }

    private:
        WorkerThread<MockWorkerThread> *m_workerThread;
        StringDataSocket* m_dataSocket = nullptr;
    };
    
}

class TestServerSingleThread : public ::testing::Test
{
public:
    TestServerSingleThread() :
                    serverSingleThreadCallback(),
                    serverThread("TestServer", serverSingleThreadCallback, "10000", getServerConfig()),
                    connectionThread("Client",mockConnectionThreadCallback, EventThreadConfig(getServerConfig())),
                    waitForCount(1u),
                    workerConsumer("TestConsumer", mockConsumerCallback)
    {
        connectionThread.connectTo("localhost", "10000");
    }
protected:
    void SetUp() override 
    {
        serverThread.connectTo(workerConsumer);
        serverThread.connectFrom(workerConsumer);
        serverThread.init();
        serverThread.start();
        serverSingleThreadCallback.setWorkerThread(workerConsumer);
        EXPECT_CALL(mockConsumerCallback,  onInit()).WillOnce(::testing::Return(true));
        EXPECT_CALL(mockConsumerCallback,  onStart()).Times(1);
        EXPECT_CALL(mockConsumerCallback,  onExit()).Times(1);
        workerConsumer.init();
        workerConsumer.start();
    }

    void TearDown() override 
    {
        serverThread.stop();
        connectionThread.stop();
        connectionThread.join();
        serverThread.join();
        workerConsumer.join();
    }

    ServerSingleThreadCallback serverSingleThreadCallback;
    ServerSingleThread<ServerSingleThreadCallback, ServerSingleThreadCallback::StringDataSocket> serverThread;

    MockConnectionThreadCallback mockConnectionThreadCallback;
    ConnectionThread<IConnectionThreadCallback, lu::platform::socket::DataSocket<IConnectionThreadCallback, lu::platform::socket::data_handler::String>> connectionThread;
    lu::utils::WaitForCount waitForCount;

    MockWorkerThread mockConsumerCallback;
    WorkerThread<MockWorkerThread> workerConsumer;
};

TEST_F(TestServerSingleThread, TestPingPong)
{
    EXPECT_CALL(mockConnectionThreadCallback,  onInit()).WillOnce(::testing::Return(true));
    EXPECT_CALL(mockConnectionThreadCallback,  onStart());
    
    EXPECT_CALL(mockConnectionThreadCallback,  onConnection(::testing::_)).Times(1).WillRepeatedly(::testing::Invoke(
        [&]( lu::platform::socket::DataSocket<IConnectionThreadCallback, lu::platform::socket::data_handler::String>& dataSocket)
        { 
            lu::platform::socket::data_handler::String::Message request("Ping");
            dataSocket.sendMsg(&request, sizeof(lu::platform::socket::data_handler::String::Message));
        }));

    EXPECT_CALL(mockConnectionThreadCallback, onData(::testing::_, ::testing::_)).Times(2).WillRepeatedly(::testing::Invoke([&](lu::platform::socket::DataSocket<IConnectionThreadCallback, lu::platform::socket::data_handler::String> &dataSocket, void *message)
        { 
            auto* strMessage = reinterpret_cast<lu::platform::socket::data_handler::String::Message*>(message);
            static int count = 0;
            count++;

            if (count == 1)
            {
                ASSERT_EQ(strMessage->getString(), "Pong");
                ASSERT_EQ(dataSocket.getPort(), 10000);
                lu::platform::socket::data_handler::String::Message request("GetName");
                dataSocket.sendMsg(&request, sizeof(lu::platform::socket::data_handler::String::Message));
            }
            else
            {
                ASSERT_EQ(strMessage->getString(), "TestConsumer");
                waitForCount.increment();
            }
            delete strMessage; 
        }));

    EXPECT_CALL(mockConsumerCallback,  onMsg(::testing::_)).WillRepeatedly(::testing::Invoke(
        [&](channel::ChannelData channelData)
        {
            auto* strMessage = reinterpret_cast<lu::platform::socket::data_handler::String::Message*>(channelData.data);
            EXPECT_EQ(strMessage->getString(), "GetName");
            auto reply = new lu::platform::socket::data_handler::String::Message(LuThread::getCurrentThreadName());
            LuThread::transferMsgToIOThread(channelData.channelID, reply);
            delete strMessage;
        }));

    EXPECT_CALL(mockConnectionThreadCallback,  onExit()).Times(1);
    EXPECT_CALL(mockConnectionThreadCallback, onTimer(::testing::_)).WillRepeatedly(testing::DoDefault());
    EXPECT_CALL(mockConnectionThreadCallback, onAppMsg(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(mockConnectionThreadCallback, onClientClose(::testing::_)).Times(0);
    connectionThread.init();
    connectionThread.start(true);
    waitForCount.wait();
    workerConsumer.stop();
}

