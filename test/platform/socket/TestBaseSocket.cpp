#include <gtest/gtest.h>
#include <platform/socket/BaseSocket.h>
#include <platform/defs.h>
#include <utils/Utils.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>


namespace
{
    template<typename T>
    T getSocketOption(int socketId, int level, int option)
    {
        assert(socketId != lu::platform::NULL_FD);
        T value;
        socklen_t len = sizeof(value);
        if (::getsockopt(socketId, level, option, &value, &len) == -1)
        {
            return T{};
        }

        return value;
    }
}

class TestDummyBaseSocket : public lu::platform::socket::BaseSocket
{
public:
    TestDummyBaseSocket(int socket) : lu::platform::socket::BaseSocket(socket)
    {}

    void onEvent(struct epoll_event& event)
    {
    }

    bool setReuseAddAndPort()
    {
        return lu::platform::socket::BaseSocket::setReuseAddAndPort();
    }

    void getIPAndPort(const struct sockaddr &address)
    {
        lu::platform::socket::BaseSocket::getIPAndPort(address);
    }

    void setSocketDescriptorFlags()
    {
        lu::platform::socket::BaseSocket::setSocketDescriptorFlags();
    }
};

class TestBaseSocket : public ::testing::Test
{
protected:
    void SetUp() override 
    {
        m_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        ASSERT_NE(m_socket, -1);
    }

    void TearDown() override 
    {
        int flags = ::fcntl(m_socket, F_GETFL);
        ASSERT_EQ(flags, lu::platform::NULL_FD);
        ASSERT_EQ(errno, EBADF); 
    }

    int m_socket;
};

TEST_F(TestBaseSocket, setNonBlocking)
{
    TestDummyBaseSocket socketObject(m_socket);
    socketObject.setNonBlocking();
    socketObject.setSocketDescriptorFlags();
    int flags = fcntl(socketObject.getFD(), F_GETFL, 0);
    ASSERT_TRUE(flags & O_NONBLOCK);
}

TEST_F(TestBaseSocket, setReuseAddAndPort)
{
    TestDummyBaseSocket socketObject(m_socket);
    socketObject.setReuseAddAndPort();
    int ret = getSocketOption<int>(socketObject.getFD(), SOL_SOCKET, SO_REUSEADDR);
    ASSERT_EQ(ret, 1);

    
}

TEST_F(TestBaseSocket, setTCPMaxSendRate)
{
    TestDummyBaseSocket socketObject(m_socket);
    socketObject.setTCPMaxSendRate(1000000);
    int ret = getSocketOption<int>(socketObject.getFD(), SOL_SOCKET, SO_MAX_PACING_RATE);
    ASSERT_EQ(ret, 1000000);
}


TEST_F(TestBaseSocket, setKeepAlive)
{
    TestDummyBaseSocket socketObject(m_socket);
    socketObject.setKeepAlive();
    int ret = getSocketOption<int>(socketObject.getFD(), SOL_SOCKET, SO_KEEPALIVE);
    ASSERT_EQ(ret, 1);
}

TEST_F(TestBaseSocket, setReceiveTimeOut)
{
    TestDummyBaseSocket socketObject(m_socket);
    timeval timeout = {1, 0};
    socketObject.setReceiveTimeOut(timeout);
    timeval ret = getSocketOption<timeval>(socketObject.getFD(), SOL_SOCKET, SO_RCVTIMEO);
    ASSERT_EQ(ret.tv_sec, timeout.tv_sec);
    ASSERT_EQ(ret.tv_usec, timeout.tv_usec);
}

TEST_F(TestBaseSocket, setSendTimeOut)
{
    TestDummyBaseSocket socketObject(m_socket);
    struct timeval timeout = {1, 0};
    socketObject.setSendTimeOut(timeout);
    timeval ret = getSocketOption<timeval>(socketObject.getFD(), SOL_SOCKET, SO_SNDTIMEO);
    ASSERT_EQ(ret.tv_sec, timeout.tv_sec);
    ASSERT_EQ(ret.tv_usec, timeout.tv_usec);
}

TEST_F(TestBaseSocket, setMinimumDataToReturnRecv)
{
    TestDummyBaseSocket socketObject(m_socket);
    socketObject.setMinimumDataToReturnRecv(1024);
    int ret = getSocketOption<int>(socketObject.getFD(), SOL_SOCKET, SO_RCVLOWAT);
    ASSERT_EQ(ret, 1024);
}



TEST_F(TestBaseSocket, setDataFlushTimeoutOnClose)
{
    TestDummyBaseSocket socketObject(m_socket);
    socketObject.setDataFlushTimeoutOnClose(1);
    int ret = getSocketOption<int>(socketObject.getFD(), SOL_SOCKET, SO_LINGER);
    ASSERT_EQ(ret, 1);
}

TEST_F(TestBaseSocket, setBufferTCPSendData)
{
    TestDummyBaseSocket socketObject(m_socket);
    socketObject.setBufferTCPSendData();
    int ret = getSocketOption<int>(socketObject.getFD(), SOL_SOCKET, TCP_NODELAY);
    ASSERT_EQ(ret, 0);
}

TEST_F(TestBaseSocket, setTCPKeepAlive)
{
    TestDummyBaseSocket socketObject(m_socket);
    socketObject.setTCPKeepAlive(1, 2, 3);   
    int ret = getSocketOption<int>(socketObject.getFD(), IPPROTO_TCP, TCP_KEEPCNT);
    ASSERT_EQ(ret, 1);
    ret = getSocketOption<int>(socketObject.getFD(), IPPROTO_TCP, TCP_KEEPIDLE);
    ASSERT_EQ(ret, 2);
    ret = getSocketOption<int>(socketObject.getFD(), IPPROTO_TCP, TCP_KEEPINTVL);
    ASSERT_EQ(ret, 3);
}

TEST_F(TestBaseSocket, setTCPNoDelay)
{
    TestDummyBaseSocket socketObject(m_socket);
    socketObject.setTCPNoDelay();
    int ret = getSocketOption<int>(socketObject.getFD(), SOL_SOCKET, TCP_NODELAY);
    ASSERT_EQ(ret, 1);
}

TEST_F(TestBaseSocket, setRxBufferSize)
{
    TestDummyBaseSocket socketObject(m_socket);
    socketObject.setRxBufferSize(1024);
    ASSERT_EQ(socketObject.getRxBufferSize(), 1024);
}

TEST_F(TestBaseSocket, setTxBufferSize)
{
    TestDummyBaseSocket socketObject(m_socket);
    socketObject.setTxBufferSize(1024);
    ASSERT_EQ(socketObject.getTxBufferSize(), 1024);
}

TEST_F(TestBaseSocket, setMinimumDataToReturnSend)
{
    TestDummyBaseSocket socketObject(m_socket);
    socketObject.setMinimumDataToReturnSend(1024);
    int ret = getSocketOption<int>(socketObject.getFD(), SOL_SOCKET, SO_SNDLOWAT);
    ASSERT_EQ(ret, 1024);
}

TEST_F(TestBaseSocket, setMaxSendDataWaitThreshold)
{
    TestDummyBaseSocket socketObject(m_socket);
    socketObject.setMaxSendDataWaitThreshold(TCP_NOTSENT_LOWAT);
    int ret = getSocketOption<int>(socketObject.getFD(), SOL_SOCKET, TCP_NOTSENT_LOWAT);
    ASSERT_EQ(ret, TCP_NOTSENT_LOWAT);
}

TEST_F(TestBaseSocket, getIPAndPort)
{
    TestDummyBaseSocket socketObject(m_socket);
    struct sockaddr address;
    socketObject.getIPAndPort(address);
    ASSERT_EQ(socketObject.getIP(), "127.0.0.1");
    ASSERT_EQ(socketObject.getPort(), 8080);
}
