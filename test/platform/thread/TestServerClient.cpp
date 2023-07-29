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


class TestServerClient : public ::testing::Test
{
public:
    TestServerClient() :
                    serverThread("TestServer", mockServerThreadCallback, "10000"),
                    connectSocket("Client",mockConnectionThreadCallback, EventThreadConfig(SeverConfig()))
    {
        connectSocket.connectTo("localhost", "1000");
    }
protected:
    void SetUp() override 
    {
        serverClientThreadsCallbacks = serverThread.getClientThreadCallbacks();

        // Server thread expected callbacks
        EXPECT_CALL(mockServerThreadCallback,  onStart());
        EXPECT_CALL(mockServerThreadCallback,  onStartComplete()).WillOnce(::testing::Invoke([&]()
            { 
                serverStarted.store(true); 
            }));
        EXPECT_CALL(mockServerThreadCallback,  onExit()).Times(1);
        EXPECT_CALL(mockServerThreadCallback, onTimer(::testing::_)).WillRepeatedly(testing::DoDefault());

        // Server's client handling threads expected callbacks
        for (auto serverClientThread: serverClientThreadsCallbacks)
        {
            auto mockServerClientCallback = reinterpret_cast<MockServerClientThreadCallback*>(serverClientThread);
            EXPECT_CALL(*mockServerClientCallback,  onInit());
            EXPECT_CALL(*mockServerClientCallback,  onStart());
            EXPECT_CALL(*mockServerClientCallback,  onStartComplete());
            EXPECT_CALL(*mockServerClientCallback,  onNewConnection(::testing::_)).WillRepeatedly(::testing::Invoke(
                [&](lu::platform::socket::DataSocket<IClientThreadCallback, lu::platform::socket::data_handler::String>* dataSocket)
                { 
                    serverClientDataSockets.insert(dataSocket);
                }));

            EXPECT_CALL(*mockServerClientCallback,  onData(::testing::_, ::testing::_)).WillRepeatedly(::testing::Invoke(
                [&]( lu::platform::socket::DataSocket<IClientThreadCallback, lu::platform::socket::data_handler::String>& dataSocket, void* message)
                { 
                    std::string* strMessage = reinterpret_cast<std::string*>(message);
                    ASSERT_EQ(*strMessage, "Ping");
                    //std::string reply("Pong");
                    //dataSocket.sendMsg(reply.data(), reply.length());
                }));
            EXPECT_CALL(*mockServerClientCallback,  onExit()).Times(1);
            EXPECT_CALL(*mockServerClientCallback, onTimer(::testing::_)).WillRepeatedly(testing::DoDefault());
        }

        {
            std::unique_lock<std::mutex> lock(startMutex);
            startCondition.wait(lock, [&] { return serverStarted.load(); });
        }

        connectSocket.init();
    }

    void TearDown() override 
    {

    }

    MockServerThreadCallback mockServerThreadCallback;
    ServerThread<IServerThreadCallback, lu::platform::socket::data_handler::String, MockServerClientThreadCallback, IClientThreadCallback> serverThread;
    std::vector<IClientThreadCallback*> serverClientThreadsCallbacks;

    std::set<lu::platform::socket::DataSocket<IClientThreadCallback, lu::platform::socket::data_handler::String>*> serverClientDataSockets;

    lu::platform::thread::MockConnectionThreadCallback mockConnectionThreadCallback;
    lu::platform::thread::ConnectionThread<lu::platform::thread::IConnectionThreadCallback, lu::platform::socket::data_handler::String> connectSocket;

    std::mutex startMutex;
    std::condition_variable startCondition;
    std::atomic<bool> serverStarted = false;
};

TEST_F(TestServerClient, TestPingPong)
{
    EXPECT_CALL(mockConnectionThreadCallback,  onInit());
    EXPECT_CALL(mockConnectionThreadCallback,  onStart());
    EXPECT_CALL(mockConnectionThreadCallback,  onStartComplete());
    EXPECT_CALL(mockConnectionThreadCallback,  onData(::testing::_, ::testing::_)).WillRepeatedly(::testing::Invoke(
        [&]( lu::platform::socket::DataSocket<IConnectionThreadCallback, lu::platform::socket::data_handler::String>& dataSocket, void* message)
        { 
            std::string* strMessage = reinterpret_cast<std::string*>(message);
            ASSERT_EQ(*strMessage, "Ping");
            //std::string reply("Pong");
            //dataSocket.sendMsg(reply.data(), reply.length());
        }));
    EXPECT_CALL(mockConnectionThreadCallback,  onExit()).Times(1);
    EXPECT_CALL(mockConnectionThreadCallback, onTimer(::testing::_)).WillRepeatedly(testing::DoDefault());
    connectSocket.start();
    std::string reply("Ping");
    //connectSocket.sendMsg(reply.data(), reply.length());
}

