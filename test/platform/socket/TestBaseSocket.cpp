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

class TestBaseSocket : public ::testing::Test
{
protected:
    void SetUp() override 
    {
        m_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        ASSERT_NE(m_socket, lu::platform::NULL_FD);
    }

    void TearDown() override 
    {
        int flags = ::fcntl(m_socket, F_GETFL);
        ASSERT_EQ(flags, lu::platform::NULL_FD);
        ASSERT_EQ(errno, EBADF); 
    }

    int m_socket;
};

TEST_F(TestBaseSocket, setNonBlockingAndBlocking)
{
    lu::platform::socket::BaseSocket socketObject(m_socket);
    socketObject.setNonBlocking();
    int flags = fcntl(socketObject.getFD(), F_GETFL, 0);
    ASSERT_TRUE(flags & O_NONBLOCK);
    socketObject.setBlocking();
    flags = fcntl(socketObject.getFD(), F_GETFL, 0);
    ASSERT_FALSE(flags & O_NONBLOCK);
}

TEST_F(TestBaseSocket, setReuseAddAndPort)
{
    lu::platform::socket::BaseSocket socketObject(m_socket);
    socketObject.setReuseAddAndPort();
    int ret = socketObject.getSocketOption<int>(SOL_SOCKET, SO_REUSEADDR);
    ASSERT_EQ(ret, 1);
}

TEST_F(TestBaseSocket, setTCPMaxSendRate)
{
    lu::platform::socket::BaseSocket socketObject(m_socket);
    socketObject.setTCPMaxSendRate(1000000);
    int ret = socketObject.getSocketOption<int>(SOL_SOCKET, SO_MAX_PACING_RATE);
    ASSERT_EQ(ret, 1000000);
}


TEST_F(TestBaseSocket, setKeepAlive)
{
    lu::platform::socket::BaseSocket socketObject(m_socket);
    socketObject.setKeepAlive();
    int ret = socketObject.getSocketOption<int>(SOL_SOCKET, SO_KEEPALIVE);
    ASSERT_EQ(ret, 1);
}

TEST_F(TestBaseSocket, setReceiveTimeOut)
{
    lu::platform::socket::BaseSocket socketObject(m_socket);
    timeval timeout = {1, 0};
    socketObject.setReceiveTimeOut(timeout);
    timeval ret = socketObject.getSocketOption<timeval>(SOL_SOCKET, SO_RCVTIMEO);
    ASSERT_EQ(ret.tv_sec, timeout.tv_sec);
    ASSERT_EQ(ret.tv_usec, timeout.tv_usec);
}

TEST_F(TestBaseSocket, setSendTimeOut)
{
    lu::platform::socket::BaseSocket socketObject(m_socket);
    struct timeval timeout = {1, 0};
    socketObject.setSendTimeOut(timeout);
    timeval ret = socketObject.getSocketOption<timeval>(SOL_SOCKET, SO_SNDTIMEO);
    ASSERT_EQ(ret.tv_sec, timeout.tv_sec);
    ASSERT_EQ(ret.tv_usec, timeout.tv_usec);
}

TEST_F(TestBaseSocket, setMinimumDataToReturnRecv)
{
    lu::platform::socket::BaseSocket socketObject(m_socket);
    socketObject.setMinimumDataToReturnRecv(1024);
    int ret = socketObject.getSocketOption<int>(SOL_SOCKET, SO_RCVLOWAT);
    ASSERT_EQ(ret, 1024);
}



TEST_F(TestBaseSocket, setDataFlushTimeoutOnClose)
{
    lu::platform::socket::BaseSocket socketObject(m_socket);
    socketObject.setDataFlushTimeoutOnClose(1);
    int ret = socketObject.getSocketOption<int>(SOL_SOCKET, SO_LINGER);
    ASSERT_EQ(ret, 1);
}

TEST_F(TestBaseSocket, setBufferTCPSendData)
{
    lu::platform::socket::BaseSocket socketObject(m_socket);
    socketObject.setBufferTCPSendData();
    int ret = socketObject.getSocketOption<int>(SOL_SOCKET, TCP_NODELAY);
    ASSERT_EQ(ret, 0);
}

TEST_F(TestBaseSocket, checkMove)
{
    lu::platform::socket::BaseSocket temSocketObj(m_socket);
    ASSERT_NE(temSocketObj.getFD(), nullptr);
    int flags = ::fcntl(m_socket, F_GETFL);
    ASSERT_NE(flags, lu::platform::NULL_FD);
    
    lu::platform::socket::BaseSocket socketObject = std::move(temSocketObj);
    ASSERT_EQ(temSocketObj.getFD(), nullptr);
    flags = ::fcntl(m_socket, F_GETFL);
    ASSERT_NE(flags, lu::platform::NULL_FD);
    ASSERT_EQ(temSocketObj.getFD(), nullptr);
    ASSERT_NE(socketObject.getFD(), nullptr);

    int socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ASSERT_NE(socket, lu::platform::NULL_FD);
    temSocketObj = lu::platform::socket::BaseSocket(socket);
    ASSERT_NE(temSocketObj.getFD(), nullptr);
    flags = ::fcntl(socket, F_GETFL);
    ASSERT_NE(flags, lu::platform::NULL_FD);

    temSocketObj = lu::platform::socket::BaseSocket();
    ASSERT_EQ(temSocketObj.getFD(), nullptr);
    flags = ::fcntl(socket, F_GETFL);
    ASSERT_EQ(flags, lu::platform::NULL_FD);
}

TEST_F(TestBaseSocket, checkClose)
{
    lu::platform::socket::BaseSocket temSocketObj(m_socket);
    ASSERT_NE(temSocketObj.getFD(), nullptr);
    int flags = ::fcntl(m_socket, F_GETFL);
    ASSERT_NE(flags, lu::platform::NULL_FD);
    temSocketObj.close();
    flags = ::fcntl(m_socket, F_GETFL);
    ASSERT_EQ(flags, lu::platform::NULL_FD);
    ASSERT_EQ(temSocketObj.getFD(), nullptr);
}


