#include <platform/socket/BaseSocket.h>
#include <platform/Defs.h>
#include <utils/Utils.h>

#include <array>

#include <arpa/inet.h>
#include <cassert>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <unistd.h>

#include <glog/logging.h>

using namespace lu::platform::socket;

namespace
{
    const lu::platform::FileDescriptor nullFileDescriptor(nullptr);
}

BaseSocket::BaseSocket(int fileDescriptor) : m_fd(new FileDescriptor(fileDescriptor))
{ 
}

BaseSocket::BaseSocket(int fileDescriptor, const sockaddr& address) : BaseSocket(fileDescriptor)
{
    getIPAndPort(address);
}

BaseSocket::BaseSocket(BaseSocket&& other) noexcept:
    m_fd(std::move(other.m_fd)),
    m_ip(std::move(other.m_ip)),
    m_port(other.m_port),
    m_socketFlags(other.m_socketFlags),
    m_reuseAddAndPort(other.m_reuseAddAndPort)
    
{
}

BaseSocket& BaseSocket::operator=(BaseSocket&& other) noexcept
{
    m_fd = std::move(other.m_fd);
    m_ip = std::move(other.m_ip);
    m_port = other.m_port;
    m_socketFlags = other.m_socketFlags;
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
    if (!m_fd->setToNonBlocking())
    {
        LOG(ERROR) << "Can not set non blocking for socket " << (int) *m_fd << "!";
    }
}

void BaseSocket::setBlocking()
{
    if (m_fd->setBlocking())
    {
        LOG(ERROR) << "Can not set blocking for socket " << (int) *m_fd << "!";
    }
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
    struct linger soLinger{};
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
        std::array<char, INET_ADDRSTRLEN> ipAddress{};
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,)
        const auto& ipv4 = reinterpret_cast<const struct sockaddr_in &>(address);
        ::inet_ntop(AF_INET, &(ipv4.sin_addr), ipAddress.data(), INET_ADDRSTRLEN );
        m_ip = std::string(ipAddress.data());
        m_port = ::ntohs(ipv4.sin_port);
    } 
    else if (address.sa_family == AF_INET6) 
    {
        std::array<char, INET6_ADDRSTRLEN> ipAddress{};
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        const auto &ipv6 = reinterpret_cast<const struct sockaddr_in6 &>(address);
        ::inet_ntop(AF_INET6, &(ipv6.sin6_addr), ipAddress.data(), INET6_ADDRSTRLEN);
        m_ip = std::string(ipAddress.data());
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
    return static_cast<bool>(::setsockopt(*m_fd, level, option, &value, sizeof(value)) != -1);
}

template<typename T>
T BaseSocket::getSocketOption(int level, int option) const
{
    assert(*m_fd != nullptr);
    T value{};
    socklen_t len = sizeof(value);
    if (::getsockopt(*m_fd, level, option, &value, &len) == -1)
    {
        return T{};
    }

    return value;
}

int BaseSocket::stop(ShutSide site) 
{
    assert(*m_fd != nullptr);
    return ::shutdown(*m_fd, site);
}

int BaseSocket::close() 
{
    assert(*m_fd != nullptr);
    return m_fd->close();
}

bool BaseSocket::readDataSocket(uint8_t *buf, size_t size, ssize_t &readCount)
{
    return lu::utils::Utils::readDataSocket(*m_fd, buf, size, readCount);
}

bool BaseSocket::readDataSocket(struct iovec *dataBufferVec, int numOfVBuffers, ssize_t &readCount)
{
    return lu::utils::Utils::readDataSocket(*m_fd, dataBufferVec, numOfVBuffers, readCount);
}

ssize_t BaseSocket::send(void *buffer, ssize_t size)
{
    if (*m_fd == nullptr)
    {
        return 0;
    }

    ssize_t totalSent = 0;
    auto uint8_tBuffer = reinterpret_cast<uint8_t *>(buffer);

    while (totalSent < size)
    {
        // TODO we can try replace this by ::writev (scatter/gather IO)
        ssize_t numBytesSend = ::send(*m_fd, uint8_tBuffer + totalSent, size, MSG_DONTWAIT);

        if (numBytesSend < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }

            return totalSent;
        }
        else if (numBytesSend != size)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR || errno == ENOBUFS)
            {
                totalSent += numBytesSend;
                size -= numBytesSend;
                continue;
            }

            return totalSent;
        }

        totalSent += numBytesSend;
    }

    return totalSent;
}

ssize_t BaseSocket::sendfile(int fileDescriptor, ssize_t size)
{
    off_t offset = 0;
    ssize_t sendBytes = ::sendfile(*m_fd, fileDescriptor, &offset, size);

    if (sendBytes == -1)
    {
        return false;
    }

    ssize_t totalSent = 0;

    while (totalSent < size)
    {
        ssize_t numBytesSend = ::sendfile(*m_fd, fileDescriptor, &offset, size);

        if (numBytesSend < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }

            return totalSent;
        }
        else if (numBytesSend != size)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR || errno == ENOBUFS)
            {
                totalSent += numBytesSend;
                size -= numBytesSend;
                continue;
            }

            return totalSent;
        }

        totalSent += numBytesSend;
    }

    return sendBytes;
}

template bool lu::platform::socket::BaseSocket::setSocketOption<int>(int level, int option, const int& value);
template int lu::platform::socket::BaseSocket::getSocketOption<int>(int level, int option) const;
template bool lu::platform::socket::BaseSocket::setSocketOption<timeval>(int level, int option, const timeval& value);
template timeval lu::platform::socket::BaseSocket::getSocketOption<timeval>(int level, int option) const;

