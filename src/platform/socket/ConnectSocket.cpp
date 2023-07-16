#include<platform/socket/ConnectSocket.h>
#include<platform/socket/IDataHandler.h>
#include <platform/defs.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <glog/logging.h>

#include <cstring>


using namespace lu::platform::socket;

template<lu::common::NonPtrClassOrStruct DataHandler>
ConnectSocket<DataHandler>::ConnectSocket(const std::string& host, const std::string& service): 
        m_dataSocket(nullptr),
        m_host(host), 
        m_service(service)
{
}

template<lu::common::NonPtrClassOrStruct DataHandler>
ConnectSocket<DataHandler>::ConnectSocket(ConnectSocket&& other) noexcept :
    m_dataSocket(std::move(other.m_dataSocket)),
    m_host(std::move(other.m_host)),
    m_service(std::move(other.m_service))
{
    other.m_dataSocket = nullptr;
}

template<lu::common::NonPtrClassOrStruct DataHandler>
ConnectSocket<DataHandler>& ConnectSocket<DataHandler>::operator=(ConnectSocket&& other) noexcept
{
    m_dataSocket = std::move(other.m_dataSocket);
    m_host = std::move(other.m_host);
    m_service = std::move(other.m_service);
    other.m_dataSocket = nullptr;
    return *this;
}

template<lu::common::NonPtrClassOrStruct DataHandler>
bool ConnectSocket<DataHandler>::connectToTCP(DataHandler& dataHandler)
{
    struct addrinfo addrCriteria;
    ::memset(&addrCriteria, 0, sizeof(addrCriteria));
    addrCriteria.ai_family = AF_UNSPEC;
    addrCriteria.ai_socktype = SOCK_STREAM;
    addrCriteria.ai_protocol = IPPROTO_TCP;
    struct addrinfo *servAddr;
    int rtnVal = ::getaddrinfo(m_host.c_str(), m_service.c_str(), &addrCriteria, &servAddr);

    if (rtnVal != 0)
    {
        LOG(ERROR) << "Can not connect to " << m_host << ":" << m_service;
        return false;
    }

    int fd = lu::platform::NULL_FD;
    struct addrinfo *connectAddr = nullptr;

    for (struct addrinfo *addr = servAddr; addr != NULL; addr = addr->ai_next) 
    {
        fd = ::socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        DLOG(INFO) << "Try connection to protocolFamily:" << addr->ai_family 
            << ", socketType:" << addr->ai_socktype << ", protocol:" << addr->ai_protocol;

        if (fd < 0)
        {
            continue;
        }

        if (::connect(fd, addr->ai_addr, addr->ai_addrlen) == 0)
        {
            connectAddr = addr;
            break;
        }
        
        ::close(fd);
        fd = lu::platform::NULL_FD;
    }

    if  (fd == lu::platform::NULL_FD)
    {
        ::freeaddrinfo(servAddr);
        return false;
    }

    m_dataSocket = new DataSocket<DataHandler>(BaseSocket(fd, *connectAddr->ai_addr), dataHandler);
    DLOG(INFO) << "Connecting to " << m_dataSocket->getIP() << ":" << m_dataSocket->getPort();
    ::freeaddrinfo(servAddr);
    return true;
}

template class lu::platform::socket::ConnectSocket<lu::platform::socket::IDataHandler>;