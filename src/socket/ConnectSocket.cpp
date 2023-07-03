#include<socket/ConnectSocket.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <cstring>
#include <glog/logging.h>

using namespace lu::socket;

ConnectSocket::ConnectSocket(const std::string& host, const std::string& service): DataSocket(NULL_SOCKET),
        m_host(host), m_service(service)
{
}

bool ConnectSocket::connectToTCP()
{
    struct addrinfo addrCriteria;
    memset(&addrCriteria, 0, sizeof(addrCriteria));
    addrCriteria.ai_family = AF_UNSPEC;
    addrCriteria.ai_socktype = SOCK_STREAM;
    addrCriteria.ai_protocol = IPPROTO_TCP;
    struct addrinfo *servAddr;
    int rtnVal = getaddrinfo(m_host.c_str(), m_service.c_str(), &addrCriteria, &servAddr);

    if (rtnVal != 0)
    {
        LOG(ERROR) << "Can not connect to " << m_host << ":" << m_service;
        return false;
    }

    for (struct addrinfo *addr = servAddr; addr != NULL; addr = addr->ai_next) 
    {
        m_socketId = ::socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        DLOG(INFO) << "Try connection to protocolFamily:" << addr->ai_family 
            << ", socketType:" << addr->ai_socktype << ", protocol:" << addr->ai_protocol;

        if (m_socketId < 0)
        {
            continue;
        }

        if (connect(m_socketId, addr->ai_addr, addr->ai_addrlen) == 0)
        {
            getIPAndPort(*addr->ai_addr);
            DLOG(INFO) << "Connecting to " << m_ip << ":" << m_port; 
            break;
        }
        
        close(m_socketId);
        m_socketId = NULL_SOCKET;
    }

    freeaddrinfo(servAddr);
    return m_socketId == NULL_SOCKET ? false : true;
}