#include <socket/ServerSocket.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <glog/logging.h>

using namespace lu::socket;

ServerSocket::ServerSocket(const std::string& service, int numberOfConnectionQueued): 
    BaseSocket(NULL_SOCKET), 
    m_service(service),
    m_numberOfConnectionQueued(numberOfConnectionQueued)
{

}

bool ServerSocket::setUpTCP()
{
    struct addrinfo addrCriteria;
    memset(&addrCriteria, 0, sizeof(addrCriteria));
    addrCriteria.ai_family = AF_UNSPEC;
    addrCriteria.ai_flags = AI_PASSIVE;
    addrCriteria.ai_socktype = SOCK_STREAM;
    addrCriteria.ai_protocol = IPPROTO_TCP;
    struct addrinfo *servAddr;
    int rtnVal = getaddrinfo(NULL, m_service.c_str(), &addrCriteria, &servAddr);

    if (rtnVal != 0)
    {
        LOG(ERROR) << "Cannot start service " << m_service;
        return false;
    }

    for (struct addrinfo *addr = servAddr; addr != NULL; addr = addr->ai_next)
    {
        m_socketId = ::socket(servAddr->ai_family, servAddr->ai_socktype, servAddr->ai_protocol);

        if (m_socketId < 0)
        {
            continue;
        }

        if (m_reuseAddAndPort)
        {
            setReuseAddAndPort();
        }

        if ((bind(m_socketId, servAddr->ai_addr, servAddr->ai_addrlen) == 0) && 
                (listen(m_socketId, m_numberOfConnectionQueued) == 0)) 
        {
            struct sockaddr_storage localAddr;
            socklen_t addrSize = sizeof(localAddr);
            
            if (getsockname(m_socketId, (struct sockaddr *) &localAddr, &addrSize) < 0)
            {
                LOG(ERROR) << "getsockname failed";
                return false;
            }

            getIPAndPort((struct sockaddr&) localAddr);
            setSocketDescriptorFlags();
            LOG(INFO) << "Service started " << m_ip << ":" << m_port;
            break;
        }

        close(m_socketId);
        m_socketId = NULL_SOCKET;
    }

    freeaddrinfo(servAddr);
    return m_socketId == NULL_SOCKET ?  false: true;
}

DataSocket* ServerSocket::acceptDataSocket()
{
    if (m_socketId == NULL_SOCKET)
    {
        return nullptr;
    }

    struct sockaddr_storage clntAddr;
    socklen_t clntAddrLen = sizeof(clntAddr);
    int clntSock = accept(m_socketId, (struct sockaddr *) &clntAddr, &clntAddrLen);

    if (clntSock < 0)
    {
        LOG(WARNING) << "Accept failed!!";
        return nullptr;
    }

    return new DataSocket(clntSock, (struct sockaddr &) clntAddr);
}
