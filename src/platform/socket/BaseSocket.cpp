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
    lu::platform::FileDescriptor nullFileDescriptor(nullptr);
}

BaseSocket::BaseSocket(int fd) : m_fd(new FileDescriptor(fd))
{
    if (m_fd == nullptr)
    {
        return;
    }

    m_socketDescriptorFlags = ::fcntl(*m_fd, F_GETFL, 0);  
}

BaseSocket::~BaseSocket()
{
}

BaseSocket::BaseSocket(int fd, const sockaddr& address) : BaseSocket(fd)
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
    m_ip = std::move(other.m_ip);
    m_port = other.m_port;
    m_socketFlags = other.m_socketFlags;
    m_socketDescriptorFlags = other.m_socketDescriptorFlags;
    m_reuseAddAndPort = other.m_reuseAddAndPort;
    return *this;
}

const lu::platform::FileDescriptor& BaseSocket::getFD() const
{
    if (m_fd == nullptr)
    {
        return nullFileDescriptor;
    }

    return *m_fd;
}

void BaseSocket::setAddress(const sockaddr& address)
{
    getIPAndPort(address);
}

void BaseSocket::setNonBlocking()
{
    if (m_fd->setToNonBlocking())
    {
        LOG(ERROR) << "Can not set non blocking for socket " << (int) *m_fd << "!";
    }
}

void BaseSocket::setBlocking()
{
    m_socketDescriptorFlags &= ~O_NONBLOCK;
}


bool BaseSocket::setReuseAddAndPort()
{
    return setSocketOption<int>(SOL_SOCKET, SO_REUSEADDR, 1);
}

bool BaseSocket::setTCPMaxSendRate(unsigned long long int rateInBitPerSec)
{
    return setSocketOption(SOL_SOCKET, SO_MAX_PACING_RATE, rateInBitPerSec);
}

bool BaseSocket::setTCPNoDelay()
{
    return setSocketOption<int>(IPPROTO_TCP, TCP_NODELAY, 1);
}

bool BaseSocket::setRxBufferSize(int size)
{
    return setSocketOption(SOL_SOCKET, SO_RCVBUF, size);
}

bool BaseSocket::setTxBufferSize(int size)
{
    return setSocketOption(SOL_SOCKET, SO_SNDBUF, size);
}

bool BaseSocket::setKeepAlive()
{
    return setSocketOption<int>(SOL_SOCKET, SO_KEEPALIVE, 1);
}

// No impact when MSG_DONTWAIT or socket is non-blocking
bool BaseSocket::setReceiveTimeOut(const struct timeval &timeout)
{
    return setSocketOption(SOL_SOCKET, SO_RCVTIMEO, timeout);
}

// No impact when MSG_DONTWAIT or socket is non-blocking
bool BaseSocket::setSendTimeOut(const struct timeval &timeout)
{
    return setSocketOption(SOL_SOCKET, SO_SNDTIMEO, timeout);
}

// No impact when MSG_DONTWAIT pass to recv
bool BaseSocket::setMinimumDataToReturnRecv(int numberOfBytes)
{
    return setSocketOption(SOL_SOCKET, SO_RCVLOWAT, numberOfBytes);
}

// No impact when MSG_DONTWAIT pass to recv
bool BaseSocket::setMinimumDataToReturnSend(int numberOfBytes)
{
    return setSocketOption( SOL_SOCKET, SO_SNDLOWAT, numberOfBytes);
}

// Wait for unsent data to be sent on close
bool BaseSocket::setDataFlushTimeoutOnClose(int waitTimeInSec)
{
    struct linger soLinger;
    soLinger.l_onoff = 1;
    soLinger.l_linger = waitTimeInSec;
    return setSocketOption(SOL_SOCKET, SO_LINGER, soLinger);
}

bool BaseSocket::setBufferTCPSendData()
{
    return setSocketOption<int>(SOL_SOCKET, TCP_NODELAY, 0);
}

bool BaseSocket::setMaxSendDataWaitThreshold(int numberOfBytes)
{
    return setSocketOption(SOL_SOCKET, TCP_NOTSENT_LOWAT, numberOfBytes);
}

bool  BaseSocket::setTCPKeepAlive(int maxIdleTime, int interval, int numberOfTry)
{
    assert(*m_fd != nullptr);
    assert(maxIdleTime>0 && interval > 0 && numberOfTry > 0);
    int keepAlive = 1;
    if (::setsockopt(*m_fd, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(keepAlive)) == -1) 
    {
        return false;
    }
    
    // Allowed time in seconds to be idle, after this idle time system sends first keep-alive probe
    if (::setsockopt(*m_fd, IPPROTO_TCP, TCP_KEEPIDLE, &maxIdleTime, sizeof(maxIdleTime)) == -1) 
    {
        return false;
    }

    // Subsequent keep-alive probe interval in seconds
    if (::setsockopt(*m_fd, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval)) == -1) 
    {
        return false;
    }

    // Number keep-alive probe attempt before closing the socket
    if (::setsockopt(*m_fd, IPPROTO_TCP, TCP_KEEPCNT, &numberOfTry, sizeof(numberOfTry)) == -1) 
    {
        return false;
    }

    return true;
}

bool BaseSocket::setSocketDescriptorFlags()
{
    assert(*m_fd != nullptr);
    int s = ::fcntl(*m_fd, F_SETFL, m_socketDescriptorFlags);

    if (s == -1) 
    {
        return false;
    }

    return true;
}

int BaseSocket::getRxBufferSize() const
{
    return getSocketOption<int>(SOL_SOCKET, SO_RCVBUF);
}

int BaseSocket::getTxBufferSize() const
{
    return getSocketOption<int>(SOL_SOCKET, SO_SNDBUF);
}

void BaseSocket::getIPAndPort(const struct sockaddr &address)
{
    
    if (address.sa_family == AF_INET) 
    {
        char ip[INET_ADDRSTRLEN];
        struct sockaddr_in& ipv4 = (struct sockaddr_in &)address;
        ::inet_ntop(AF_INET, &(ipv4.sin_addr), ip, INET_ADDRSTRLEN );
        m_ip = std::string(ip);
        m_port = ::ntohs(ipv4.sin_port);
    } 
    else if (address.sa_family == AF_INET6) 
    {
        char ip[INET6_ADDRSTRLEN];
        struct sockaddr_in6 &ipv6 = (struct sockaddr_in6 &)address;
        ::inet_ntop(AF_INET6, &(ipv6.sin6_addr), ip, INET6_ADDRSTRLEN);
        m_ip = std::string(ip);
        m_port = ::ntohs(ipv6.sin6_port);
    } 
    else 
    {
        LOG(WARNING) << "Unknown address family!";
        return;
    }

    
}


template<typename T>
bool BaseSocket::setSocketOption(int level, int option, const T& value)
{
    assert(*m_fd != nullptr);
    if (::setsockopt(*m_fd, level, option, &value, sizeof(value)) == -1) 
    {
        return false;
    }

    return true;
}

template<typename T>
T BaseSocket::getSocketOption(int level, int option) const
{
    assert(*m_fd != nullptr);
    T value;
    socklen_t len = sizeof(value);
    if (::getsockopt(*m_fd, level, option, &value, &len) == -1)
    {
        return T{};
    }

    return value;
}

void BaseSocket::stop() 
{
    assert(*m_fd != nullptr);
    shutdown(*m_fd,SHUT_RDWR);
}


template bool lu::platform::socket::BaseSocket::setSocketOption<int>(int level, int option, const int& value);
template int lu::platform::socket::BaseSocket::getSocketOption<int>(int level, int option) const;
template bool lu::platform::socket::BaseSocket::setSocketOption<timeval>(int level, int option, const timeval& value);
template timeval lu::platform::socket::BaseSocket::getSocketOption<timeval>(int level, int option) const;

