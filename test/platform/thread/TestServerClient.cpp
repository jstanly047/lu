#include <platform/thread/ServerClientThread.h>
#include <platform/thread/ServerThread.h>
#include <platform/socket/data_handler/String.h>
#include <platform/socket/ConnectSocket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>

#include <thread>
#include <condition_variable>

#include <platform/thread/MockServerClientThreadCallback.h>
#include <platform/thread/MockServerThreadCallback.h>
#include <platform/socket/MockDataSocketCallback.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace lu::platform::thread;


class TestServerClient : public ::testing::Test
{
public:
    TestServerClient() :
                    serverThread("TestServer", mockServerThreadCallback, "10000"),
                    connectSocket("localhost", "10000")
    {

    }
protected:
    void SetUp() override 
    {
        serverClientThreads = &(serverThread.getSeverClientThreads());

        // Server thread expected callbacks
        EXPECT_CALL(mockServerThreadCallback,  onStart());
        EXPECT_CALL(mockServerThreadCallback,  onStartComplete()).WillOnce(::testing::Invoke([&]()
            { 
                serverStarted.store(true); 
            }));
        EXPECT_CALL(mockServerThreadCallback,  onExit()).Times(1);
        EXPECT_CALL(mockServerThreadCallback, onTimer(::testing::_)).WillRepeatedly(testing::DoDefault());

        // Server's client handling threads expected callbacks
        for (auto& serverClientThread: *serverClientThreads)
        {
            auto& mockServerClientCallback = serverClientThread.getServerClientThreadCallback();
            EXPECT_CALL(mockServerClientCallback,  onInit());
            EXPECT_CALL(mockServerClientCallback,  onStart());
            EXPECT_CALL(mockServerClientCallback,  onNewConnection(::testing::_)).WillRepeatedly(::testing::Invoke(
                [&](lu::platform::socket::DataSocket<MockServerClientThreadCallback, lu::platform::socket::data_handler::String>* dataSocket)
                { 
                    serverClientDataSockets.insert(dataSocket);
                }));

            EXPECT_CALL(mockServerClientCallback,  onNewConnection(::testing::_)).WillRepeatedly(::testing::Invoke(
                [&](lu::platform::socket::DataSocket<MockServerClientThreadCallback, lu::platform::socket::data_handler::String>* dataSocket)
                { 
                    serverClientDataSockets.erase(dataSocket);
                    delete dataSocket;
                }));

            EXPECT_CALL(mockServerClientCallback,  onMessage(::testing::_)).WillRepeatedly(::testing::Invoke(
                [&]( void* message)
                { 
                    std::string* strMessage = reinterpret_cast<std::string*>(message);
                    ASSERT_EQ(*strMessage, "Ping");
                    //std::string reply("Pong");
                    //dataSocket.sendMsg(reply.data(), reply.length());
                }));
            EXPECT_CALL(mockServerClientCallback,  onExit()).Times(1);
            EXPECT_CALL(mockServerClientCallback, onTimer(::testing::_)).WillRepeatedly(testing::DoDefault());
        }

        {
            std::unique_lock<std::mutex> lock(startMutex);
            startCondition.wait(lock, [&] { return serverStarted.load(); });
        }
    }

    void TearDown() override 
    {

    }

    MockServerThreadCallback mockServerThreadCallback;
    ServerThread<MockServerThreadCallback, MockServerClientThreadCallback, lu::platform::socket::data_handler::String> serverThread;
    std::vector<ServerClientThread<MockServerClientThreadCallback, lu::platform::socket::data_handler::String> >* serverClientThreads;

    std::set<lu::platform::socket::DataSocket<MockServerClientThreadCallback, lu::platform::socket::data_handler::String>*> serverClientDataSockets;

    lu::platform::socket::MockDataSocketCallback mockDataSocketCallback;
    lu::platform::socket::ConnectSocket<lu::platform::socket::MockDataSocketCallback, lu::platform::socket::data_handler::String> connectSocket;

    std::mutex startMutex;
    std::condition_variable startCondition;
    std::atomic<bool> serverStarted = false;
};

TEST_F(TestServerClient, TestPingPong)
{
    EXPECT_CALL(mockDataSocketCallback, onData(::testing::_)).WillOnce(::testing::Invoke(
                [&]( void* message)
                { 
                    std::string* strMessage = reinterpret_cast<std::string*>(message);
                    ASSERT_EQ(*strMessage, "Pong");
                }));
    ASSERT_TRUE(connectSocket.connectToTCP(mockDataSocketCallback));
    std::string reply("Ping");
    connectSocket.sendMsg(reply.data(), reply.length());
}

