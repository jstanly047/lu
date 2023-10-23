#include <platform/socket/ServerSocket.h>
#include <platform/socket/ConnectSocket.h>
#include <platform/socket/DataSocket.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>

#include <thread>
#include <condition_variable>

using namespace lu::platform::socket;

class MockSeverSocketCallback 
{
public:
    MOCK_METHOD(void, onNewConnection, (lu::platform::socket::BaseSocket *));
};

class DataHandler
{
public:
    DataHandler() {}

    virtual uint8_t *getReceiveBufferToFill() { return nullptr; }
    virtual std::size_t getReceiveBufferSize() { return 0; }
    virtual std::size_t getHeaderSize() { return 0; }
    virtual std::size_t readHeader(std::size_t offset) { return offset; }
    virtual void *readMessage([[maybe_unused]] std::size_t offset, [[maybe_unused]] std::size_t size) { return nullptr; }

private:
};

class DataSocketCallback
{
public:
    virtual void onClientClose([[maybe_unused]] DataSocket<DataSocketCallback, DataHandler> &dataSocket) {}
    virtual void onData([[maybe_unused]] DataSocket<DataSocketCallback, DataHandler> &dataSocket, [[maybe_unused]] void *data) {}
};

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
        serverThead.join();
    }

    void accept()
    {
        {
            std::lock_guard<std::mutex> lock(startMutex);
            threadStarted = true;
            startCondition.notify_one();
        }
       
        auto  dataSocket = serverSocket.acceptDataSocket();
        m_clientSockets.emplace_back(dataSocket);
        EXPECT_NE((int) dataSocket->getFD(), lu::platform::NULL_FD);
    }

    MockSeverSocketCallback m_mockServerSocketCallback;
    DataSocketCallback m_clientDataSocketCallback;
    ConnectSocket<DataSocketCallback, DataSocket<DataSocketCallback, DataHandler>> connectSocket;
    ServerSocket<MockSeverSocketCallback> serverSocket;
    std::thread serverThead;
    std::vector<std::unique_ptr<BaseSocket>> m_clientSockets;
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

TEST_F(TestSocket, DISABLED_setMinimumDataToReturnSend)
{
    auto& socketObject = *connectSocket.getBaseSocket();
    socketObject.setMinimumDataToReturnSend(8);
    int ret = socketObject.getSocketOption<int>(SOL_SOCKET, SO_SNDLOWAT);
    ASSERT_EQ(ret, 181242);
}

TEST_F(TestSocket, DISABLED_setMaxSendDataWaitThreshold)
{
    auto& socketObject = *connectSocket.getBaseSocket();
    socketObject.setMaxSendDataWaitThreshold(512000);
    int ret = socketObject.getSocketOption<int>(SOL_SOCKET, TCP_NOTSENT_LOWAT);
    ASSERT_EQ(ret, 512000);
}

TEST_F(TestSocket, checkDestructionOfConnectionDataSocket)
{
    auto connectSocketTemp = new ConnectSocket<DataSocketCallback, DataSocket<DataSocketCallback, DataHandler>>("localhost", "10000");
    connectSocketTemp->connectToTCP(m_clientDataSocketCallback);
    ASSERT_NE(connectSocketTemp->getBaseSocket(), nullptr);
    int fd = connectSocketTemp->getBaseSocket()->getFD();
    delete connectSocketTemp;
    auto flags = ::fcntl(fd, F_GETFL);
    ASSERT_EQ(flags, lu::platform::NULL_FD);
}

TEST_F(TestSocket, checkOnReconnectCloseAlreadyOpenedDataSocket)
{
    ConnectSocket<DataSocketCallback, DataSocket<DataSocketCallback, DataHandler>> connectSocketTemp("localhost", "10000");
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
