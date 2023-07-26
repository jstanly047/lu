#include <platform/socket/ServerSocket.h>
#include <platform/socket/ConnectSocket.h>
#include <platform/socket/IDataHandler.h>
#include <platform/socket/IServerSocketCallback.h>
#include <platform/socket/IDataSocketCallback.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>

#include <thread>
#include <condition_variable>

#include <platform/socket/MockSeverSocketCallback.h>

using namespace lu::platform::socket;


class TestSocket : public ::testing::Test
{
public:
    TestSocket() :  connectSocket("localhost", "10000"),
                    serverSocket("10000", m_mockServerSocketCallback, true)
    {

    }
protected:
    void SetUp() override 
    {
        auto result = serverSocket.setUpTCP(4);
        EXPECT_TRUE(result);
        ASSERT_EQ(serverSocket.getBaseSocket().getIP(), "0.0.0.0");
        ASSERT_EQ(serverSocket.getBaseSocket().getPort(), 10000);
        ASSERT_EQ(serverSocket.getService(), "10000");
        int flags = ::fcntl(serverSocket.getFD(), F_GETFL, 0);
        ASSERT_TRUE(!(flags & O_NONBLOCK));
        serverThead = std::thread(&TestSocket::accept, this);

        {
            std::unique_lock<std::mutex> lock(startMutex);
            startCondition.wait(lock, [&] { return threadStarted; });
        }

        //EXPECT_CALL(mockDataHandler, onClientClose).Times(0);
        //EXPECT_CALL(mockDataHandler, onNewConnection).Times(0);
        EXPECT_EQ(connectSocket.getBaseSocket(), nullptr);
        result = connectSocket.connectToTCP(m_clientDataSocketCallback);
        EXPECT_TRUE(result);
        EXPECT_EQ(connectSocket.getHost(), "localhost");
        EXPECT_EQ(connectSocket.getService(), "10000");
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
        EXPECT_NE((int) dataSocket->getFD(), lu::platform::NULL_FD);
    }

    MockSeverSocketCallback m_mockServerSocketCallback;
    IDataSocketCallback<IDataHandler> m_clientDataSocketCallback;
    ConnectSocket<IDataSocketCallback<IDataHandler>, IDataHandler> connectSocket;
    ServerSocket<IServerSocketCallback> serverSocket;
    std::thread serverThead;
    BaseSocket* dataSocket = nullptr;
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

    BaseSocket& serverBaseSocket = serverSocket.getBaseSocket();
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

TEST_F(TestSocket, checkDestructionOfConnectionDataSocket)
{
    auto connectSocketTemp = new ConnectSocket<IDataSocketCallback<IDataHandler>, IDataHandler>("localhost", "10000");
    connectSocketTemp->connectToTCP(m_clientDataSocketCallback);
    ASSERT_NE(connectSocketTemp->getBaseSocket(), nullptr);
    int fd = connectSocketTemp->getBaseSocket()->getFD();
    delete connectSocketTemp;
    auto flags = ::fcntl(fd, F_GETFL);
    ASSERT_EQ(flags, lu::platform::NULL_FD);
}

TEST_F(TestSocket, checkOnReconnectCloseAlreadyOpenedDataSocket)
{
    ConnectSocket<IDataSocketCallback<IDataHandler>, IDataHandler> connectSocketTemp("localhost", "10000");
    connectSocketTemp.connectToTCP(m_clientDataSocketCallback);
    ASSERT_NE(connectSocketTemp.getBaseSocket(), nullptr);
    int fd = connectSocketTemp.getBaseSocket()->getFD();
    connectSocketTemp.connectToTCP(m_clientDataSocketCallback);
    ASSERT_NE(connectSocketTemp.getBaseSocket(), nullptr);
    auto flags = ::fcntl(fd, F_GETFL);
    ASSERT_EQ(flags, lu::platform::NULL_FD);
}

TEST_F(TestSocket, testMoveConstructAndOperatorConnectionSocket)
{
    auto dataSocket = connectSocket.getBaseSocket();
    int fd = dataSocket->getFD();
    auto connectSocketTemp = std::move(connectSocket);
    ASSERT_EQ(connectSocketTemp.getBaseSocket()->getFD(), dataSocket->getFD());
    connectSocket = std::move(connectSocketTemp);
    ASSERT_EQ(connectSocketTemp.getBaseSocket(), nullptr);
    ASSERT_EQ(connectSocket.getBaseSocket()->getFD(), dataSocket->getFD());
    auto flags = ::fcntl(fd, F_GETFL);
    ASSERT_NE(flags, lu::platform::NULL_FD);
}

TEST_F(TestSocket, testMoveConstructAndOperatorServiceSocket)
{
    auto& serverSocketBase = serverSocket.getBaseSocket();
    int fd = serverSocketBase.getFD();
    auto serverSocketTemp = std::move(serverSocket);
    ASSERT_EQ((int)serverSocketTemp.getBaseSocket().getFD(), fd);
    serverSocket = std::move(serverSocketTemp);
    ASSERT_EQ(serverSocketTemp.getBaseSocket().getFD(), nullptr);
    ASSERT_EQ((int)serverSocket.getBaseSocket().getFD(), fd);
    auto flags = ::fcntl(fd, F_GETFL);
    ASSERT_NE(flags, lu::platform::NULL_FD);
}
