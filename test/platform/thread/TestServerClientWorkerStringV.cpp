#include <platform/thread/ClientThread.h>
#include <platform/thread/ConnectionThread.h>
#include <platform/thread/MockServerThreadCallback.h>
#include <platform/thread/MockWorkerThread.h>
#include <platform/thread/ServerThread.h>
#include <platform/thread/WorkerThread.h>
#include <platform/socket/DataSocketV.h>
#include <platform/socket/data_handler/StringV.h>


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
    constexpr unsigned int NUM_MSG_SEND = 1000u;

    ServerConfig getServerConfig()
    {
        ServerConfig serverConfig{};
        serverConfig.TIMER_IN_MSEC = 100u;
        serverConfig.CREATE_NEW_THREAD = true;
        return serverConfig;
    }

    lu::utils::WaitForCount waitForCount(1u);

    class TestClientTheadCallback
    {
    public:
        using StringDataSocket=lu::platform::socket::DataSocketV<TestClientTheadCallback, lu::platform::socket::data_handler::StringV>;
        TestClientTheadCallback() : m_workerThread(nullptr) {}

        bool onInit() { return true; }
        void onStart() {}
        void onExit() { ASSERT_EQ(m_expectedMsg, NUM_MSG_SEND);}

        void onNewConnection(StringDataSocket &dataSocket)
        {
            dataSocket.setCustomObjectPtr(this);
        }

        void onAppMsg([[maybe_unused]] void *msg, [[maybe_unused]] lu::platform::thread::channel::ChannelID channelID)
        {
        }

        void onTimer(const lu::platform::FDTimer<TestClientTheadCallback> &)
        {
        }

        void onClientClose(StringDataSocket &dataSocket)
        {
            EXPECT_EQ(dataSocket.getCustomObjectPtr(), this);
        }

        void onData(StringDataSocket &dataSocket, void *message)
        {
            auto *strMessage = reinterpret_cast<lu::platform::socket::data_handler::StringV::Message *>(message);

            if (strMessage->getString() == "Ping")
            {
                lu::platform::socket::data_handler::StringV::Message reply("Pong");
                dataSocket.sendMsg(&reply, sizeof(lu::platform::socket::data_handler::StringV::Message));
                delete strMessage;
            }
            else if (strMessage->getString() == "GetName")
            {
                lu::platform::socket::data_handler::StringV::Message reply(LuThread::getCurrentThreadName());
                dataSocket.sendMsg(&reply, sizeof(lu::platform::socket::data_handler::StringV::Message));
                delete strMessage;
            }
            else
            {
                m_expectedMsg++;
                ASSERT_EQ(m_expectedMsg, std::stoul(strMessage->getString()));
                LuThread::transferMsg(m_workerThread->getName(), strMessage);

                if (m_expectedMsg == NUM_MSG_SEND)
                {
                    dataSocket.stop(lu::platform::socket::ShutdownWrite);
                }
            }
        }

        void setWorkerThread(WorkerThread<MockWorkerThread>& workerThread)
        {
            m_workerThread = &workerThread;
        }

    private:
        WorkerThread<MockWorkerThread> *m_workerThread;
        unsigned int m_expectedMsg{};
    };


    class ConnectionThreadCallback
    {
    public:
         using StringDataSocket=lu::platform::socket::DataSocketV<ConnectionThreadCallback, lu::platform::socket::data_handler::StringV>;
        bool onInit() { return true; }
        void onStart() {}
        void onAppMsg([[maybe_unused]]void*, [[maybe_unused]]lu::platform::thread::channel::ChannelID channelID) {}

        // EXPECT_CALL(connectionThreadCallback,  onStartComplete());
        void onConnection(StringDataSocket &dataSocket)
        {
            lu::platform::socket::data_handler::StringV::Message request("Ping");
            dataSocket.sendMsg(&request, sizeof(lu::platform::socket::data_handler::StringV::Message));
            clientSideDataSocketV.push_back({&dataSocket, 0});
        }

        void onData(StringDataSocket &dataSocket, void *message)
        {
            auto *strMessage = reinterpret_cast<lu::platform::socket::data_handler::StringV::Message *>(message);

            for (unsigned int i = 0; i < clientSideDataSocketV.size(); i++)
            {
                if (clientSideDataSocketV[i].first == &dataSocket)
                {
                    if (clientSideDataSocketV[i].second == 0)
                    {
                        ASSERT_EQ(strMessage->getString(), "Pong");
                        ASSERT_EQ(dataSocket.getPort(), 10000);
                        clientSideDataSocketV[i].second++;
                        lu::platform::socket::data_handler::StringV::Message request("GetName");
                        dataSocket.sendMsg(&request, sizeof(lu::platform::socket::data_handler::StringV::Message));
                    }
                    else if (clientSideDataSocketV[i].second == 1)
                    {
                        ASSERT_EQ(strMessage->getString(), expected[i]);
                        std::thread myThread(&ConnectionThreadCallback::sendTestMessages, this, &dataSocket);
                        m_createdThreads.push_back(std::move(myThread));
                    }

                    break;
                }
            }

            delete strMessage;
        }

        void sendTestMessages(StringDataSocket *dataSocket)
        {
            for (unsigned int i = 1; i <= NUM_MSG_SEND; i++)
            {
                lu::platform::socket::data_handler::StringV::Message request(std::to_string(i));
                dataSocket->sendMsg(&request, sizeof(lu::platform::socket::data_handler::StringV::Message));
            }
        }

        void onClientClose([[maybe_unused]]StringDataSocket &dataSocket)
        {
        }

        void onExit() 
        {
            ASSERT_EQ(m_createdThreads.size(), 2u);

            for (auto &t : m_createdThreads)
            {
                t.join();
            }
        }

        void onTimer(const lu::platform::FDTimer<ConnectionThreadCallback> &) {}

    private:
        std::vector<std::pair<StringDataSocket *, int>> clientSideDataSocketV;
        std::vector<std::thread> m_createdThreads;
        std::vector<std::string> expected = {"TestServer_1", "TestServer_2"};
    };

    class ServerThreadCallback
    {
    public:
        bool onInit() { return true; }
        void onStart() {}
        void onExit() {}
        void onTimer(const lu::platform::FDTimer<ServerThreadCallback> &) {}
    };
}

class TestServerClientWorkerStringV : public ::testing::Test
{
public:
    TestServerClientWorkerStringV() :
                    serverThread("TestServer", serverThreadCallback, "10000", getServerConfig()),
                    connectionThread("Client",connectionThreadCallback, EventThreadConfig(getServerConfig())),
                    workerConsumer("TestConsumer", mockConsumerCallback)
    {
        connectionThread.connectTo("localhost", "10000", false);
        connectionThread.connectTo("localhost", "10000", false);
    }
protected:
    void SetUp() override 
    {
        EXPECT_EQ(serverThread.getName(),"TestServer");
        serverThread.connect(workerConsumer);
        
        for (auto& callbacks : serverThread.getClientThreadCallbacks())
        {
            callbacks->setWorkerThread(workerConsumer);
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

    ServerThreadCallback serverThreadCallback;
    ServerThread<ServerThreadCallback, TestClientTheadCallback::StringDataSocket, TestClientTheadCallback> serverThread;
    ConnectionThreadCallback connectionThreadCallback;
    ConnectionThread<ConnectionThreadCallback, ConnectionThreadCallback::StringDataSocket> connectionThread;

    MockWorkerThread mockConsumerCallback;
    WorkerThread<MockWorkerThread> workerConsumer;
};

TEST_F(TestServerClientWorkerStringV, TestPingPong)
{
    EXPECT_CALL(mockConsumerCallback,  onMsg(::testing::_)).WillRepeatedly(::testing::Invoke(
        [&](channel::ChannelData channelData)
        {            
            auto* strMessage = reinterpret_cast<lu::platform::socket::data_handler::StringV::Message*>(channelData.data);

            static unsigned int stopCount = 0;
            if (std::stoul(strMessage->getString()) == NUM_MSG_SEND)
            {
                stopCount++;
            }
            
            delete strMessage;

            if (stopCount == 2u)
            {
                workerConsumer.stop();
                waitForCount.increment();
            }
        }));

    connectionThread.init();
    connectionThread.start(true);
    waitForCount.wait();
}