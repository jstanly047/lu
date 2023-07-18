#include <platform/socket/ServerSocket.h>
#include <platform/socket/ConnectSocket.h>
#include <platform/socket/IDataHandler.h>
#include <platform/socket/IConnectionHandler.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>

#include <thread>
#include <condition_variable>

// Define a mock DataHandler class
class MockDataHandler : public lu::platform::socket::IConnectionHandler, public lu::platform::socket::IDataHandler
{
public:
    MOCK_METHOD(uint8_t*, getReceiveBufferToFill, (),(override));
    MOCK_METHOD(std::size_t, getReceiveBufferSize, (), (override));
    MOCK_METHOD(std::size_t, getHeaderSize, (), (override));
    MOCK_METHOD(std::size_t, readHeader, (std::size_t), (override));
    MOCK_METHOD(void, readMessage, (std::size_t, std::size_t), (override));
    MOCK_METHOD(void, onClose, (lu::platform::socket::DataSocket<lu::platform::socket::IDataHandler>*), (override));
    MOCK_METHOD(void, onNewConnection, (lu::platform::socket::BaseSocket*), (override));
};


class TestSocket : public ::testing::Test
{
public:
    TestSocket() :  connectSocket("localhost", "10000"),
                    serverSocket("10000", mockDataHandler, true)
    {

    }
protected:
    void SetUp() override 
    {
        auto result = serverSocket.setUpTCP(4);
        EXPECT_TRUE(result);
        ASSERT_EQ(serverSocket.getBaseSocket().getIP(), "0.0.0.0");
        ASSERT_EQ(serverSocket.getBaseSocket().getPort(), 10000);
        int flags = ::fcntl(serverSocket.getBaseSocket().getFD(), F_GETFL, 0);
        ASSERT_TRUE(!(flags & O_NONBLOCK));
        serverThead = std::thread(&TestSocket::accept, this);

        {
            std::unique_lock<std::mutex> lock(startMutex);
            startCondition.wait(lock, [&] { return threadStarted; });
        }

        EXPECT_CALL(mockDataHandler, getReceiveBufferToFill).Times(0);
        EXPECT_CALL(mockDataHandler, getReceiveBufferSize).WillRepeatedly(::testing::Return(512));
        EXPECT_CALL(mockDataHandler, getHeaderSize).WillRepeatedly(::testing::Return(64));
        EXPECT_CALL(mockDataHandler, readHeader).Times(0);
        EXPECT_CALL(mockDataHandler, readMessage).Times(0);
        EXPECT_CALL(mockDataHandler, onClose).Times(0);
        EXPECT_CALL(mockDataHandler, onNewConnection).Times(0);
        result = connectSocket.connectToTCP(mockDataHandler);
        EXPECT_TRUE(result);
            
    }

    void TearDown() override 
    {
        delete dataSocket;
        dataSocket = nullptr;
        serverThead.join();
    }

    void accept()
    {
        {
            std::lock_guard<std::mutex> lock(startMutex);
            threadStarted = true;
            startCondition.notify_one();
        }
       
        dataSocket = serverSocket.acceptDataSocket();

    }

    MockDataHandler mockDataHandler;
    lu::platform::socket::ConnectSocket<lu::platform::socket::IDataHandler> connectSocket;
    lu::platform::socket::ServerSocket<lu::platform::socket::IConnectionHandler> serverSocket;
    std::thread serverThead;
    lu::platform::socket::BaseSocket* dataSocket = nullptr;
    std::mutex startMutex;
    std::condition_variable startCondition;
    bool threadStarted = false;
};

TEST_F(TestSocket, setTCPKeepAlive)
{
    auto& socketObject = *connectSocket.getBaseSocket();
    socketObject.setTCPKeepAlive(1, 2, 3);   
    int ret = socketObject.getSocketOption<int>(IPPROTO_TCP, TCP_KEEPIDLE);
    ASSERT_EQ(ret, 1);
    ret = socketObject.getSocketOption<int>(IPPROTO_TCP, TCP_KEEPINTVL);
    ASSERT_EQ(ret, 2);
    ret = socketObject.getSocketOption<int>(IPPROTO_TCP, TCP_KEEPCNT);
    ASSERT_EQ(ret, 3);
    ret = socketObject.getSocketOption<int>(SOL_SOCKET, SO_KEEPALIVE);
    ASSERT_EQ(ret, 1);

    lu::platform::socket::BaseSocket& serverBaseSocket = serverSocket.getBaseSocket();
    serverBaseSocket.setTCPKeepAlive(1, 2, 3);  
    ret = serverBaseSocket.getSocketOption<int>(IPPROTO_TCP, TCP_KEEPIDLE);
    ASSERT_EQ(ret, 1);
    ret = serverBaseSocket.getSocketOption<int>(IPPROTO_TCP, TCP_KEEPINTVL);
    ASSERT_EQ(ret, 2);
    ret = serverBaseSocket.getSocketOption<int>(IPPROTO_TCP, TCP_KEEPCNT);
    ASSERT_EQ(ret, 3);
    ret = serverBaseSocket.getSocketOption<int>(SOL_SOCKET, SO_KEEPALIVE);
    ASSERT_EQ(ret, 1);
}

TEST_F(TestSocket, setTCPNoDelay)
{
    auto& socketObject = *connectSocket.getBaseSocket();
    socketObject.setTCPNoDelay();
    int ret = socketObject.getSocketOption<int>(IPPROTO_TCP, TCP_NODELAY);
    ASSERT_EQ(ret, 1);
}

TEST_F(TestSocket, setRxBufferSize)
{
    auto& socketObject = *connectSocket.getBaseSocket();
    socketObject.setRxBufferSize(512);
    ASSERT_EQ(socketObject.getRxBufferSize(), 2304);
}

TEST_F(TestSocket, setTxBufferSize)
{
    auto& socketObject = *connectSocket.getBaseSocket();
    socketObject.setTxBufferSize(512);
    ASSERT_EQ(socketObject.getTxBufferSize(), 4608);
}

TEST_F(TestSocket, setMinimumDataToReturnSend)
{
    auto& socketObject = *connectSocket.getBaseSocket();
    socketObject.setMinimumDataToReturnSend(8);
    int ret = socketObject.getSocketOption<int>(SOL_SOCKET, SO_SNDLOWAT);
    ASSERT_EQ(ret, 181242);
}

TEST_F(TestSocket, setMaxSendDataWaitThreshold)
{
    auto& socketObject = *connectSocket.getBaseSocket();
    socketObject.setMaxSendDataWaitThreshold(512000);
    int ret = socketObject.getSocketOption<int>(SOL_SOCKET, TCP_NOTSENT_LOWAT);
    ASSERT_EQ(ret, 512000);
}