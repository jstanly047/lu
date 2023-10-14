#include <platform/thread/MockConnectionThreadCallback.h>
#include <platform/thread/ClientThread.h>
#include <platform/thread/ConnectionThread.h>
#include <platform/thread/MockServerClientThreadCallback.h>
#include <platform/thread/MockServerThreadCallback.h>
#include <platform/thread/MockWorkerThread.h>
#include <platform/thread/ServerThread.h>
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
}

using StringDataSocket=lu::platform::socket::DataSocket<IConnectionThreadCallback, lu::platform::socket::data_handler::String>;


class TestClientTheadCallback
{
public:
    using StringDataSocket=lu::platform::socket::DataSocket<TestClientTheadCallback, lu::platform::socket::data_handler::String>;
    TestClientTheadCallback() : m_thread(nullptr), m_workerThread(nullptr) {}
    
    void connectWorkerThread(WorkerThread<MockWorkerThread>& workerThread)
    {
        m_workerThread = &workerThread;
        m_thread->connect(workerThread);
    }

    bool onInit() { return true; }
    void onStart(){}
    void onExit() {}

    void onNewConnection(StringDataSocket *dataSocket)
    {
        m_clients.emplace_back(dataSocket);
    }

    void onTimer(const lu::platform::FDTimer<TestClientTheadCallback> &)
    {

    }

    void onClientClose(StringDataSocket & dataSocket)
    {
        delete &dataSocket;
    }

    void onData(StringDataSocket &dataSocket, void * message)
    {
        auto *strMessage = reinterpret_cast<lu::platform::socket::data_handler::String::Message *>(message);

        if (strMessage->getString() == "Ping")
        {
            lu::platform::socket::data_handler::String::Message reply("Pong");
            dataSocket.sendMsg(&reply, sizeof(lu::platform::socket::data_handler::String::Message));
            getThread().transferMsg(m_workerThread->getName(), strMessage);
        }
        else if (strMessage->getString() == "GetName")
        {
            lu::platform::socket::data_handler::String::Message reply(LuThread::getCurrentThreadName());
            dataSocket.sendMsg(&reply, sizeof(lu::platform::socket::data_handler::String::Message));
            delete strMessage;
        }
    }

    void setThread(LuThread &thread) { m_thread = &thread; }
    LuThread &getThread() { return *m_thread; }

private:
    LuThread *m_thread;
    WorkerThread<MockWorkerThread>* m_workerThread;
    std::vector<std::unique_ptr<StringDataSocket>> m_clients;
};

class TestServerClientWorker : public ::testing::Test
{
public:
    TestServerClientWorker() :
                    serverThread("TestServer", mockServerThreadCallback, "10000", getServerConfig()),
                    connectionThread("Client",mockConnectionThreadCallback, EventThreadConfig(getServerConfig())),
                    workerConsumer("TestConsumer", mockConsumerCallback),
                    waitForCount(1u)
    {
        connectionThread.connectTo("localhost", "10000");
        connectionThread.connectTo("localhost", "10000");
        connectionThread.connectTo("localhost", "10000");
    }
protected:
    void SetUp() override 
    {
        EXPECT_EQ(serverThread.getName(),"TestServer");
        EXPECT_CALL(mockServerThreadCallback,  onInit()).WillOnce(::testing::Return(true));
        EXPECT_CALL(mockServerThreadCallback,  onStart()).Times(1);
        EXPECT_CALL(mockServerThreadCallback,  onExit()).Times(1);
        EXPECT_CALL(mockServerThreadCallback, onTimer(::testing::_)).WillRepeatedly(testing::DoDefault());

        for (auto& callbacks : serverThread.getClientThreadCallbacks())
        {
            callbacks->connectWorkerThread(workerConsumer);
        }

        serverThread.init();
        serverThread.start();

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


    MockServerThreadCallback mockServerThreadCallback;
    ServerThread<IServerThreadCallback, TestClientTheadCallback::StringDataSocket, TestClientTheadCallback> serverThread;
    MockConnectionThreadCallback mockConnectionThreadCallback;
    ConnectionThread<IConnectionThreadCallback, StringDataSocket> connectionThread;

    MockWorkerThread mockConsumerCallback;
    WorkerThread<MockWorkerThread> workerConsumer;
    lu::utils::WaitForCount waitForCount;
};

TEST_F(TestServerClientWorker, TestPingPong)
{
    waitForCount.update(2u);
    EXPECT_CALL(mockConnectionThreadCallback,  onInit()).WillOnce(::testing::Return(true));
    EXPECT_CALL(mockConnectionThreadCallback,  onStart());
    std::vector<std::pair<StringDataSocket*, int>> clientSideDataSocket;
    std::vector<std::string> expected = {"TestServer_1", "TestServer_2", "TestServer_1"};
    //EXPECT_CALL(mockConnectionThreadCallback,  onStartComplete());
    EXPECT_CALL(mockConnectionThreadCallback,  onConnection(::testing::_)).Times(3).WillRepeatedly(::testing::Invoke(
        [&]( StringDataSocket& dataSocket)
        { 
            lu::platform::socket::data_handler::String::Message request("Ping");
            dataSocket.sendMsg(&request, sizeof(lu::platform::socket::data_handler::String::Message));
            clientSideDataSocket.push_back({&dataSocket, 0});
        }));

    EXPECT_CALL(mockConnectionThreadCallback,  onData(::testing::_, ::testing::_)).WillRepeatedly(::testing::Invoke(
        [&]( StringDataSocket& dataSocket, void* message)
        { 
            auto* strMessage = reinterpret_cast<lu::platform::socket::data_handler::String::Message*>(message);

            for (unsigned int i = 0; i < clientSideDataSocket.size(); i++)
            {
                if (clientSideDataSocket[i].first == &dataSocket)
                {
                    if (clientSideDataSocket[i].second == 0)
                    {
                        ASSERT_EQ(strMessage->getString(), "Pong");
                        ASSERT_EQ(dataSocket.getPort(), 10000);
                        clientSideDataSocket[i].second++;
                        lu::platform::socket::data_handler::String::Message request("GetName");
                        dataSocket.sendMsg(&request, sizeof(lu::platform::socket::data_handler::String::Message));
                    }
                    else
                    {
                        ASSERT_EQ(strMessage->getString(), expected[i]);
                        static unsigned int nameReplyCount = 0u;
                        ++nameReplyCount;
                        if (nameReplyCount == 3u)
                        {
                            waitForCount.increment();
                        }

                    }

                    break;
                }
            }

            delete strMessage;
        }));
    EXPECT_CALL(mockConnectionThreadCallback,  onExit()).Times(1);
    EXPECT_CALL(mockConnectionThreadCallback, onTimer(::testing::_)).WillRepeatedly(testing::DoDefault());

    EXPECT_CALL(mockConsumerCallback,  onMsg(::testing::_)).WillRepeatedly(::testing::Invoke(
        [&](channel::ChannelData channelData)
        {
            if (channelData.data == nullptr)
            {
                EXPECT_EQ(channelData.channelID, workerConsumer.getChannelID());
                waitForCount.increment();
                return;
            }

            static unsigned stopCount = 0;
            stopCount++;
            auto* strMessage = reinterpret_cast<lu::platform::socket::data_handler::String::Message*>(channelData.data);
            ASSERT_EQ(strMessage->getString(), "Ping");
            delete strMessage;

            if (stopCount == 3u)
            {
                workerConsumer.stop();
            }
        }));

    connectionThread.init();
    connectionThread.start(true);
    waitForCount.wait();
}
