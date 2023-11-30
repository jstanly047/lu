#include <platform/thread/ClientThread.h>
#include <platform/thread/ConnectionThread.h>
#include <platform/thread/MockServerThreadCallback.h>
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
    constexpr unsigned int NUM_MSG_SEND = 1000u;

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

    lu::utils::WaitForCount waitForCount(1u);

    class TestClientTheadCallback
    {
    public:
        using StringDataSocket=lu::platform::socket::DataSocket<TestClientTheadCallback, lu::platform::socket::data_handler::String>;
        TestClientTheadCallback() : m_workerThread(nullptr) {}

        bool onInit() { return true; }
        void onStart() {}
        void onExit() { ASSERT_EQ(m_expectedMsg, NUM_MSG_SEND);}

        void onNewConnection([[maybe_unused]]StringDataSocket &dataSocket)
        {
        }

        void onAppMsg(void *msg)
        {
            auto replyMsg = reinterpret_cast<ReplyMsg*>(msg);
            m_expectedValues[replyMsg->channelID]++;
            EXPECT_EQ(replyMsg->intValue, m_expectedValues[replyMsg->channelID]);
            delete replyMsg;
        }

        void onTimer(const lu::platform::FDTimer<TestClientTheadCallback> &)
        {
        }

        void onClientClose([[maybe_unused]]StringDataSocket &dataSocket)
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
                lu::platform::socket::data_handler::String::Message reply(LuThread::getCurrentThreadName());
                dataSocket.sendMsg(&reply, sizeof(lu::platform::socket::data_handler::String::Message));
                delete strMessage;
            }
            else
            {
                m_expectedMsg++;
                ASSERT_EQ(m_expectedMsg, std::stoul(strMessage->getString()));
                LuThread::transferMsg(m_workerThread->getName(), strMessage);
            }
        }

        void setWorkertThread(WorkerThread<MockWorkerThread>& workerThread)
        {
            m_workerThread = &workerThread;
        }

    private:
        WorkerThread<MockWorkerThread> *m_workerThread;
        std::map<channel::ChannelID, unsigned int> m_expectedValues;
        unsigned int m_expectedMsg{};
    };


    class ConnectionThreadCallback
    {
    public:
        using StringDataSocket=lu::platform::socket::DataSocket<ConnectionThreadCallback, lu::platform::socket::data_handler::String>;
        bool onInit() { return true; }
        void onStart() {}

        // EXPECT_CALL(connectionThreadCallback,  onStartComplete());
        void onConnection(StringDataSocket &dataSocket)
        {
            lu::platform::socket::data_handler::String::Message request("Ping");
            dataSocket.sendMsg(&request, sizeof(lu::platform::socket::data_handler::String::Message));
            clientSideDataSocket.push_back({&dataSocket, 0});
        }

        

        void onData(StringDataSocket &dataSocket, void *message)
        {
            auto *strMessage = reinterpret_cast<lu::platform::socket::data_handler::String::Message *>(message);

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
                    else if (clientSideDataSocket[i].second == 1)
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
                lu::platform::socket::data_handler::String::Message request(std::to_string(i));
                dataSocket->sendMsg(&request, sizeof(lu::platform::socket::data_handler::String::Message));
            }
        }

        void onClientClose([[maybe_unused]]StringDataSocket &dataSocket)
        {
        }

        void onExit() 
        {
            ASSERT_EQ(m_createdThreads.size(), 1u);

            for (auto &t : m_createdThreads)
            {
                t.join();
            }
        }

        void onTimer(const lu::platform::FDTimer<ConnectionThreadCallback> &) {}

    private:
        std::vector<std::pair<StringDataSocket *, int>> clientSideDataSocket;
        std::vector<std::thread> m_createdThreads;
        std::vector<std::string> expected = {"TestServer_1", "TestServer_2", "TestServer_1"};
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

class TestServerClientWorkerString : public ::testing::Test
{
public:
    TestServerClientWorkerString() :
                    serverThread("TestServer", serverThreadCallback, "10000", getServerConfig()),
                    connectionThread("Client",connectionThreadCallback, EventThreadConfig(getServerConfig())),
                    workerConsumer("TestConsumer", mockConsumerCallback)
    {
        connectionThread.connectTo("localhost", "10000");
    }
protected:
    void SetUp() override 
    {
        EXPECT_EQ(serverThread.getName(),"TestServer");
        serverThread.connectTo(workerConsumer);
        serverThread.connectFrom(workerConsumer);
        
        for (auto& callbacks : serverThread.getClientThreadCallbacks())
        {
            callbacks->setWorkertThread(workerConsumer);
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

TEST_F(TestServerClientWorkerString, TestPingPong)
{
    EXPECT_CALL(mockConsumerCallback,  onMsg(::testing::_)).WillRepeatedly(::testing::Invoke(
        [&](channel::ChannelData channelData)
        {
            if (channelData.data == nullptr)
            {
                EXPECT_EQ(channelData.channelID, workerConsumer.getChannelID());
                waitForCount.increment();
                return;
            }

            
            auto* strMessage = reinterpret_cast<lu::platform::socket::data_handler::String::Message*>(channelData.data);

            static unsigned int stopCount = 0;
            auto replyMsg = new ReplyMsg();
            replyMsg->intValue = std::stoul(strMessage->getString());
            replyMsg->channelID = LuThread::getCurrentThreadChannelID();
            if (replyMsg->intValue == NUM_MSG_SEND)
            {
                stopCount++;
            }

            LuThread::transferMsgToServerThread(channelData.channelID, replyMsg);
            
            delete strMessage;

            if (stopCount == 1u)
            {
                workerConsumer.stop();
            }
        }));

    connectionThread.init();
    connectionThread.start(true);
    waitForCount.wait();
}