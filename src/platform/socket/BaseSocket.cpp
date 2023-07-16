#include <platform/socket/BaseSocket.h>
#include <platform/defs.h>
#include <utils/Utils.h>
#include <glog/logging.h>

#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>

using namespace lu::platform::socket;

namespace
{
    template<typename T>
    bool setSocketOption(int socketId, int level, int option, const T& value)
    {
        assert(socketId != lu::platform::NULL_FD);
        if (::setsockopt(socketId, level, option, &value, sizeof(value)) == -1) 
        {
            return false;
        }

        return true;
    }

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

BaseSocket::BaseSocket(int socketId) : m_fd(socketId)
{
}

BaseSocket::~BaseSocket()
{
}

BaseSocket::BaseSocket(int socketId, const sockaddr& address) : m_fd(socketId)
{
    getIPAndPort(address);
}

BaseSocket::BaseSocket(BaseSocket&& other) noexcept:
    m_fd(std::move(other.m_fd)),
    m_ip(std::move(other.m_ip)),
    m_port(std::move(other.m_port)),
    m_socketFlags(std::move(other.m_socketFlags)),
    m_socketDescriptorFlags(std::move(other.m_socketDescriptorFlags)),
    m_reuseAddAndPort(std::move(other.m_reuseAddAndPort))
    
{
}

BaseSocket& BaseSocket::operator=(BaseSocket&& other) noexcept
{
    m_fd = std::move(other.m_fd);
    std::string m_ip = std::move(other.m_ip);
    m_port = other.m_port;
    m_socketFlags = other.m_socketFlags;
    m_socketDescriptorFlags = other.m_socketDescriptorFlags;
    m_reuseAddAndPort = other.m_reuseAddAndPort;
    return *this;
}

void BaseSocket::setNonBlocking()
{
    m_socketDescriptorFlags |= O_NONBLOCK;
}

bool BaseSocket::setReuseAddAndPort()
{
    return setSocketOption<int>(m_fd, SOL_SOCKET, SO_REUSEADDR, 1);
}

bool BaseSocket::setTCPMaxSendRate(unsigned long long int rateInBitPerSec)
{
    return setSocketOption(m_fd, SOL_SOCKET, SO_MAX_PACING_RATE, rateInBitPerSec);
}

bool BaseSocket::setTCPNoDelay()
{
    return setSocketOption<int>(m_fd, SOL_SOCKET, TCP_NODELAY, 1);
}

bool BaseSocket::setRxBufferSize(int size)
{
    return setSocketOption(m_fd,  SOL_SOCKET, SO_RCVBUF, size);
}

bool BaseSocket::setTxBufferSize(int size)
{
    return setSocketOption(m_fd,  SOL_SOCKET, SO_SNDBUF, size);
}

bool BaseSocket::setKeepAlive()
{
    return setSocketOption<int>(m_fd,  SOL_SOCKET, SO_KEEPALIVE, 1);
}

// No impact when MSG_DONTWAIT or socket is non-blocking
bool BaseSocket::setReceiveTimeOut(const struct timeval &timeout)
{
    return setSocketOption(m_fd,  SOL_SOCKET, SO_RCVTIMEO, timeout);
}

// No impact when MSG_DONTWAIT or socket is non-blocking
bool BaseSocket::setSendTimeOut(const struct timeval &timeout)
{
    return setSocketOption(m_fd,  SOL_SOCKET, SO_SNDTIMEO, timeout);
}

// No impact when MSG_DONTWAIT pass to recv
bool BaseSocket::setMinimumDataToReturnRecv(int numberOfBytes)
{
    return setSocketOption(m_fd,  SOL_SOCKET, SO_RCVLOWAT, numberOfBytes);
}

// No impact when MSG_DONTWAIT pass to recv
bool BaseSocket::setMinimumDataToReturnSend(int numberOfBytes)
{
    return setSocketOption(m_fd,  SOL_SOCKET, SO_SNDLOWAT, numberOfBytes);
}

// Wait for unsent data to be sent on close
bool BaseSocket::setDataFlushTimeoutOnClose(int waitTimeInSec)
{
    struct linger soLinger;
    soLinger.l_onoff = 1;
    soLinger.l_linger = waitTimeInSec;
    return setSocketOption(m_fd,  SOL_SOCKET, SO_LINGER, soLinger);
}

bool BaseSocket::setBufferTCPSendData()
{
    return setSocketOption<int>(m_fd,  SOL_SOCKET, TCP_NODELAY, 0);
}

bool BaseSocket::setMaxSendDataWaitThreshold(int numberOfBytes)
{
    return setSocketOption(m_fd,  SOL_SOCKET, TCP_NOTSENT_LOWAT, numberOfBytes);
}

bool  BaseSocket::setTCPKeepAlive(int maxIdleTime, int interval, int numberOfTry)
{
    assert(m_fd != lu::platform::NULL_FD);
    assert(maxIdleTime>0 && interval > 0 && numberOfTry > 0);
    int keepAlive = 1;
    if (::setsockopt(m_fd, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(keepAlive)) == -1) 
    {
        return false;
    }
    
    // Allowed time in seconds to be idle, after this idle time system sends first keep-alive probe
    if (::setsockopt(m_fd, IPPROTO_TCP, TCP_KEEPIDLE, &maxIdleTime, sizeof(maxIdleTime)) == -1) 
    {
        return false;
    }

    // Subsequent keep-alive probe interval in seconds
    if (::setsockopt(m_fd, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval)) == -1) 
    {
        return false;
    }

    // Number keep-alive probe attempt before closing the socket
    if (::setsockopt(m_fd, IPPROTO_TCP, TCP_KEEPCNT, &numberOfTry, sizeof(numberOfTry)) == -1) 
    {
        return false;
    }

    return true;
}

bool BaseSocket::setSocketDescriptorFlags()
{
    assert(m_fd != lu::platform::NULL_FD);
    int flags = ::fcntl(m_fd, F_GETFL, 0);

    if (flags == -1) 
    {
        return false;
    }

    flags |= m_socketDescriptorFlags;
    int s = ::fcntl(m_fd, F_SETFL, flags);

    if (s == -1) 
    {
        return false;
    }

    return true;
}

int BaseSocket::getRxBufferSize() const
{
    return getSocketOption<int>(m_fd, SOL_SOCKET, SO_RCVBUF);
}

int BaseSocket::getTxBufferSize() const
{
    return getSocketOption<int>(m_fd, SOL_SOCKET, SO_SNDBUF);
}

void BaseSocket::getIPAndPort(const struct sockaddr &address)
{
    char ip[INET6_ADDRSTRLEN];
    if (address.sa_family == AF_INET) 
    {
        struct sockaddr_in& ipv4 = (struct sockaddr_in &)address;
        ::inet_ntop(AF_INET, &(ipv4.sin_addr), ip, INET6_ADDRSTRLEN);
        m_port = ::ntohs(ipv4.sin_port);
    } 
    else if (address.sa_family == AF_INET6) 
    {
        struct sockaddr_in6 &ipv6 = (struct sockaddr_in6 &)address;
        ::inet_ntop(AF_INET6, &(ipv6.sin6_addr), ip, INET6_ADDRSTRLEN);
        m_port = ::ntohs(ipv6.sin6_port);
    } 
    else 
    {
        LOG(WARNING) << "Unknown address family!";
        return;
    }

    m_ip = ip;
}
