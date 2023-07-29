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
        return serverConfig;
    }
}

template class ServerThread<IServerThreadCallback, lu::platform::socket::data_handler::String, MockServerClientThreadCallback, IClientThreadCallback> ;
template class ConnectionThread<IConnectionThreadCallback, lu::platform::socket::data_handler::String>;

class TestServerClient : public ::testing::Test
{
public:
    TestServerClient() :
                    serverThread("TestServer", mockServerThreadCallback, "10000", getServerConfig()),
                    connectionThread("Client",mockConnectionThreadCallback, EventThreadConfig(getServerConfig()))
    {
        connectionThread.connectTo("localhost", "10000");
    }
protected:
    void SetUp() override 
    {
        serverClientThreadsCallbacks = serverThread.getClientThreadCallbacks();

        // Server thread expected callbacks
        EXPECT_CALL(mockServerThreadCallback,  onInit()).WillOnce(::testing::Return(true));;
        EXPECT_CALL(mockServerThreadCallback,  onStart()).WillOnce(::testing::Invoke([&]()
            { 
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
                [&](lu::platform::socket::DataSocket<IClientThreadCallback, lu::platform::socket::data_handler::String>* dataSocket)
                { 
                    serverClientDataSockets.insert(dataSocket);
                }));

            EXPECT_CALL(*mockServerClientCallback,  onData(::testing::_, ::testing::_)).WillRepeatedly(::testing::Invoke(
                [&]( lu::platform::socket::DataSocket<IClientThreadCallback, lu::platform::socket::data_handler::String>& dataSocket, void* message)
                { 
                    auto* strMessage = reinterpret_cast<lu::platform::socket::data_handler::String::Message*>(message);
                    ASSERT_EQ(strMessage->getString(), "Ping");
                    lu::platform::socket::data_handler::String::Message reply("Pong");
                    dataSocket.sendMsg(&reply, sizeof(lu::platform::socket::data_handler::String::Message));
                    delete strMessage;
                }));
            EXPECT_CALL(*mockServerClientCallback,  onExit()).Times(1);
            EXPECT_CALL(*mockServerClientCallback, onTimer(::testing::_)).WillRepeatedly(testing::DoDefault());
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
    ServerThread<IServerThreadCallback, lu::platform::socket::data_handler::String, MockServerClientThreadCallback, IClientThreadCallback> serverThread;
    std::vector<IClientThreadCallback*> serverClientThreadsCallbacks;

    std::set<lu::platform::socket::DataSocket<IClientThreadCallback, lu::platform::socket::data_handler::String>*> serverClientDataSockets;

    MockConnectionThreadCallback mockConnectionThreadCallback;
    ConnectionThread<IConnectionThreadCallback, lu::platform::socket::data_handler::String> connectionThread;

    std::mutex startMutex;
    std::condition_variable startCondition;
    std::atomic<bool> serverStarted = false;
};

TEST_F(TestServerClient, TestPingPong)
{
    EXPECT_CALL(mockConnectionThreadCallback,  onInit()).WillOnce(::testing::Return(true));
    EXPECT_CALL(mockConnectionThreadCallback,  onStart());
    //EXPECT_CALL(mockConnectionThreadCallback,  onStartComplete());
    EXPECT_CALL(mockConnectionThreadCallback,  onConnection(::testing::_)).WillRepeatedly(::testing::Invoke(
        [&]( lu::platform::socket::ConnectSocket<IConnectionThreadCallback, lu::platform::socket::data_handler::String>& dataSocket)
        { 
            lu::platform::socket::data_handler::String::Message request("Ping");
            dataSocket.sendMsg(&request, sizeof(lu::platform::socket::data_handler::String::Message));
        }));
    EXPECT_CALL(mockConnectionThreadCallback,  onData(::testing::_, ::testing::_)).WillRepeatedly(::testing::Invoke(
        [&]( lu::platform::socket::DataSocket<IConnectionThreadCallback, lu::platform::socket::data_handler::String>& dataSocket, void* message)
        { 
            auto* strMessage = reinterpret_cast<lu::platform::socket::data_handler::String::Message*>(message);
            ASSERT_EQ(strMessage->getString(), "Pong");
            ASSERT_EQ(dataSocket.getPort(), 10000);
            delete strMessage;
        }));
    EXPECT_CALL(mockConnectionThreadCallback,  onExit()).Times(1);
    EXPECT_CALL(mockConnectionThreadCallback, onTimer(::testing::_)).WillRepeatedly(testing::DoDefault());
    connectionThread.init();
    connectionThread.start(true);
    sleep(1);
}

