#include <platform/thread/MockConnectionThreadCallback.h>
#include <platform/thread/ClientThread.h>
#include <platform/thread/ConnectionThread.h>
#include <platform/thread/MockServerClientThreadCallback.h>
#include <platform/thread/MockServerThreadCallback.h>
#include <platform/thread/ServerThread.h>


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
        serverConfig.IPV6 = true;
        return serverConfig;
    }
}

template class ServerThread<IServerThreadCallback, lu::platform::socket::DataSocket<IClientThreadCallback, lu::platform::socket::data_handler::String>, MockServerClientThreadCallback, IClientThreadCallback> ;
template class ConnectionThread<IConnectionThreadCallback, lu::platform::socket::DataSocket<IConnectionThreadCallback, lu::platform::socket::data_handler::String>>;

class TestServerClient_IP6 : public ::testing::Test
{
public:
    TestServerClient_IP6() :
                    serverThread("TestServer", mockServerThreadCallback, "10000", getServerConfig()),
                    connectionThread("Client",mockConnectionThreadCallback, EventThreadConfig(getServerConfig()))
    {
        connectionThread.connectTo("::1", "10000");
        connectionThread.connectTo("::1", "10000");
        connectionThread.connectTo("::1", "10000");
    }
protected:
    void SetUp() override 
    {
        EXPECT_EQ(serverThread.getName(),"TestServer");
        serverClientThreadsCallbacks = serverThread.getClientThreadCallbacks();

        // Server thread expected callbacks
        EXPECT_CALL(mockServerThreadCallback,  onInit()).WillOnce(::testing::Return(true));
        EXPECT_CALL(mockServerThreadCallback,  onStart()).WillOnce(::testing::Invoke([&]()
            { 
                EXPECT_EQ(LuThread::getCurrentThreadName() ,"TestServer");
                std::lock_guard<std::mutex> lock(startMutex);
                serverStarted = true;
                startCondition.notify_one();
            }));
        /*EXPECT_CALL(mockServerThreadCallback,  onStartComplete()).WillOnce(::testing::Invoke([&]()
            { 
                serverStarted.store(true); 
            }));*/
        EXPECT_CALL(mockServerThreadCallback,  onExit()).Times(1);
        EXPECT_CALL(mockServerThreadCallback, onTimer(::testing::_)).WillRepeatedly(testing::DoDefault());

        // Server's client handling threads expected callbacks
        for (auto serverClientThread: serverClientThreadsCallbacks)
        {
            MockServerClientThreadCallback* mockServerClientCallback = reinterpret_cast<MockServerClientThreadCallback*>(serverClientThread);
            EXPECT_CALL(*mockServerClientCallback,  onInit()).WillOnce(::testing::Return(true));
            EXPECT_CALL(*mockServerClientCallback,  onStart());
            //EXPECT_CALL(*mockServerClientCallback,  onStartComplete());
            EXPECT_CALL(*mockServerClientCallback,  onNewConnection(::testing::_)).WillRepeatedly(::testing::Invoke(
                [&]([[maybe_unused]]lu::platform::socket::DataSocket<IClientThreadCallback, lu::platform::socket::data_handler::String>& dataSocket)
                {
                    
                }));


            EXPECT_CALL(*mockServerClientCallback,  onAppMsg(::testing::_)).Times(0);

            EXPECT_CALL(*mockServerClientCallback,  onData(::testing::_, ::testing::_)).WillRepeatedly(::testing::Invoke(
                [&]( lu::platform::socket::DataSocket<IClientThreadCallback, lu::platform::socket::data_handler::String>& dataSocket, void* message)
                { 
                    auto* strMessage = reinterpret_cast<lu::platform::socket::data_handler::String::Message*>(message);

                    if (strMessage->getString() == "Ping_1")
                    {
                        countReceivedPing[0] += 1;
                        lu::platform::socket::data_handler::String::Message reply("Pong");
                        dataSocket.sendMsg(&reply, sizeof(lu::platform::socket::data_handler::String::Message));
                    }
                    else if (strMessage->getString() == "Ping_2")
                    {
                        countReceivedPing[1] += 1;
                        lu::platform::socket::data_handler::String::Message reply("Pong");
                        dataSocket.sendMsg(&reply, sizeof(lu::platform::socket::data_handler::String::Message));
                    }
                    else if (strMessage->getString() == "Ping_3")
                    {
                        countReceivedPing[2] += 1;
                        lu::platform::socket::data_handler::String::Message reply("Pong");
                        dataSocket.sendMsg(&reply, sizeof(lu::platform::socket::data_handler::String::Message));
                    }
                    else if (strMessage->getString() == "GetName")
                    {
                        lu::platform::socket::data_handler::String::Message reply(LuThread::getCurrentThreadName());
                        dataSocket.sendMsg(&reply, sizeof(lu::platform::socket::data_handler::String::Message));
                        dataSocket.stop(lu::platform::socket::ShutdownWrite);
                    }

                    delete strMessage;
                }));
            EXPECT_CALL(*mockServerClientCallback,  onExit()).Times(1);
            EXPECT_CALL(*mockServerClientCallback, onTimer(::testing::_)).WillRepeatedly(testing::DoDefault());
            EXPECT_CALL(*mockServerClientCallback, onClientClose(::testing::_)).WillRepeatedly(testing::DoDefault());
        }

        serverThread.init();
        serverThread.start();

        {
            std::unique_lock<std::mutex> lock(startMutex);
            startCondition.wait(lock, [&] { return serverStarted.load(); });
        }
    }

    void TearDown() override 
    {
        serverThread.stop();
        connectionThread.stop();
        connectionThread.join();
        serverThread.join();
    }

    MockServerThreadCallback mockServerThreadCallback;
    ServerThread<IServerThreadCallback, lu::platform::socket::DataSocket<IClientThreadCallback, lu::platform::socket::data_handler::String>, MockServerClientThreadCallback, IClientThreadCallback> serverThread;
    std::vector<IClientThreadCallback*> serverClientThreadsCallbacks;

    MockConnectionThreadCallback mockConnectionThreadCallback;
    ConnectionThread<IConnectionThreadCallback, lu::platform::socket::DataSocket<IConnectionThreadCallback, lu::platform::socket::data_handler::String>> connectionThread;

    std::mutex startMutex;
    std::condition_variable startCondition;
    std::atomic<bool> serverStarted = false;
    std::array<std::atomic<int>, 3> countReceivedPing = {0, 0, 0}; 
};

TEST_F(TestServerClient_IP6, TestPingPong)
{
    EXPECT_CALL(mockConnectionThreadCallback,  onInit()).WillOnce(::testing::Return(true));
    EXPECT_CALL(mockConnectionThreadCallback,  onStart());
    std::vector<std::pair<lu::platform::socket::DataSocket<IConnectionThreadCallback, lu::platform::socket::data_handler::String>*, int>> clientSideDataSocket;
    std::vector<std::string> expected = {"TestServer_1", "TestServer_2", "TestServer_1"};
    //EXPECT_CALL(mockConnectionThreadCallback,  onStartComplete());
    EXPECT_CALL(mockConnectionThreadCallback,  onConnection(::testing::_)).Times(3).WillRepeatedly(::testing::Invoke(
        [&]( lu::platform::socket::DataSocket<IConnectionThreadCallback, lu::platform::socket::data_handler::String>& dataSocket)
        { 
            static int pingNumber = 1;
            lu::platform::socket::data_handler::String::Message request("Ping_" + std::to_string(pingNumber));
            dataSocket.sendMsg(&request, sizeof(lu::platform::socket::data_handler::String::Message));
            clientSideDataSocket.push_back({&dataSocket, 0});
            pingNumber++;
        }));

    EXPECT_CALL(mockConnectionThreadCallback,  onData(::testing::_, ::testing::_)).WillRepeatedly(::testing::Invoke(
        [&]( lu::platform::socket::DataSocket<IConnectionThreadCallback, lu::platform::socket::data_handler::String>& dataSocket, void* message)
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
                    }

                    break;
                }
            }

            delete strMessage;
        }));
    EXPECT_CALL(mockConnectionThreadCallback,  onExit()).Times(1);
    EXPECT_CALL(mockConnectionThreadCallback, onTimer(::testing::_)).WillRepeatedly(testing::DoDefault());
    EXPECT_CALL(mockConnectionThreadCallback, onAppMsg(::testing::_)).Times(0);
    EXPECT_CALL(mockConnectionThreadCallback, onClientClose(::testing::_)).Times(3);
    connectionThread.init();
    connectionThread.start(true);
    
    sleep(1);
    for (const auto& element : countReceivedPing) {
        EXPECT_EQ(element, 1);
    }
}

