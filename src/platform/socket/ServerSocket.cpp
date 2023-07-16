#include <platform/socket/ServerSocket.h>
#include <platform/socket/IDataHandler.h>
#include <platform/socket/IConnectionHandler.h>
#include <platform/defs.h>
#include <glog/logging.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include <netdb.h>


using namespace lu::platform::socket;

template<lu::common::NonPtrClassOrStruct ConnectionHandler>
ServerSocket<ConnectionHandler>::ServerSocket(const std::string& service, ConnectionHandler &connectionHandler, bool reuseAddAndPort): 
    m_baseSocket(lu::platform::NULL_FD), 
    m_connectionHandler(connectionHandler),
    m_service(service),
    m_reuseAddAndPort(reuseAddAndPort)
{

}

template<lu::common::NonPtrClassOrStruct ConnectionHandler>
ServerSocket<ConnectionHandler>::ServerSocket(ServerSocket&& other) noexcept : 
    m_baseSocket(std::move(other.m_baseSocket)),
    m_connectionHandler(other.m_connectionHandler),
    m_service(std::move(other.m_service)),
    m_reuseAddAndPort(std::move(other.m_reuseAddAndPort))
{
}

template<lu::common::NonPtrClassOrStruct ConnectionHandler>
ServerSocket<ConnectionHandler>& ServerSocket<ConnectionHandler>::operator=(ServerSocket&& other) noexcept
{
    m_baseSocket = std::move(other.m_baseSocket);
    m_connectionHandler = other.m_connectionHandler;
    m_service = std::move(other.m_service);
    m_reuseAddAndPort = other.m_reuseAddAndPort;
    return *this;
}

template<lu::common::NonPtrClassOrStruct ConnectionHandler>
bool ServerSocket<ConnectionHandler>::setUpTCP(int numberOfConnectionInWaitQueue)
{
    struct addrinfo addrCriteria;
    ::memset(&addrCriteria, 0, sizeof(addrCriteria));
    addrCriteria.ai_family = AF_UNSPEC;
    addrCriteria.ai_flags = AI_PASSIVE;
    addrCriteria.ai_socktype = SOCK_STREAM;
    addrCriteria.ai_protocol = IPPROTO_TCP;
    struct addrinfo *servAddr;
    int rtnVal = ::getaddrinfo(NULL, m_service.c_str(), &addrCriteria, &servAddr);

    if (rtnVal != 0)
    {
        LOG(ERROR) << "Cannot start service " << m_service << "!!";
        return false;
    }

    int fd = NULL_FD;
    struct sockaddr_storage localAddr;

    for (struct addrinfo *addr = servAddr; addr != NULL; addr = addr->ai_next)
    {
        fd = ::socket(servAddr->ai_family, servAddr->ai_socktype, servAddr->ai_protocol);

        

        if (fd < 0)
        {
            continue;
        }

        BaseSocket baseSocket(fd);

        if (m_reuseAddAndPort)
        {
            baseSocket.setReuseAddAndPort();
        }

        if ((::bind(fd, servAddr->ai_addr, servAddr->ai_addrlen) == 0) && 
                (::listen(fd, numberOfConnectionInWaitQueue) == 0))
        {
            
            socklen_t addrSize = sizeof(localAddr);
            
            if (::getsockname(fd, (struct sockaddr *) &localAddr, &addrSize) < 0)
            {
                LOG(ERROR) << "getsockname failed!!";
                return false;
            }

            
            break;
        }

        ::close(fd);
        fd = lu::platform::NULL_FD;
    }

    if (fd == lu::platform::NULL_FD)
    {
        ::freeaddrinfo(servAddr);
        return false;
    }

    m_baseSocket = BaseSocket(fd, (struct sockaddr&) localAddr);
    m_baseSocket.setSocketDescriptorFlags();
    LOG(INFO) << "Service started " << m_baseSocket.getIP() << ":" << m_baseSocket.getPort();
    return true;
}

template<lu::common::NonPtrClassOrStruct ConnectionHandler>
BaseSocket* ServerSocket<ConnectionHandler>::acceptDataSocket()
{
    if (m_baseSocket.getFD() == lu::platform::NULL_FD )
    {
        return nullptr;
    }

    struct sockaddr_storage clntAddr;
    socklen_t clntAddrLen = sizeof(clntAddr);
    int fd = ::accept(m_baseSocket.getFD(), (struct sockaddr *) &clntAddr, &clntAddrLen);

    if (fd < 0)
    {
        LOG(ERROR) << "Accept failed!!";
        return nullptr;
    }

    return new BaseSocket(fd, (struct sockaddr &) clntAddr);
}

template<lu::common::NonPtrClassOrStruct ConnectionHandler>
void ServerSocket<ConnectionHandler>::onEvent(struct ::epoll_event& event)
{
    if ((event.events & EPOLLERR) || !(event.events & EPOLLIN))
    {
        return;
    }

    m_connectionHandler.onNewConnection(acceptDataSocket());
}

template class lu::platform::socket::ServerSocket<lu::platform::socket::IConnectionHandler>;