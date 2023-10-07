#pragma once
#include "BaseSocket.h"
#include <common/TemplateConstraints.h>
#include <platform/IFDEventHandler.h>
#include <platform/defs.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <csignal>

#include <glog/logging.h>



namespace lu::platform::socket
{
    template<lu::common::NonPtrClassOrStruct ServerSocketCallback>
    class ServerSocket : public lu::platform::IFDEventHandler
    {
    public:
        ServerSocket(const ServerSocket&)               = delete;
        ServerSocket& operator=(const ServerSocket&)    = delete;

        ServerSocket(const std::string& service, ServerSocketCallback &connectionHandler, bool reuseAddAndPort = true)
            :
            m_baseSocket(), 
            m_serverSocketCallback(connectionHandler),
            m_service(service),
            m_reuseAddAndPort(reuseAddAndPort)
        {

        }

        ServerSocket(ServerSocket&& other) noexcept
            :
            m_baseSocket(std::move(other.m_baseSocket)),
            m_serverSocketCallback(other.m_serverSocketCallback),
            m_service(std::move(other.m_service)),
            m_reuseAddAndPort(std::move(other.m_reuseAddAndPort))
        {

        }

        ServerSocket& operator=(ServerSocket&& other) noexcept
        {
            m_baseSocket = std::move(other.m_baseSocket);
            //m_serverSocketCallback = other.m_serverSocketCallback;
            m_service = std::move(other.m_service);
            m_reuseAddAndPort = other.m_reuseAddAndPort;
            return *this;
        }

        virtual ~ServerSocket() {}

        bool setUpTCP(int numberOfConnectionInWaitQueue)
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
                LOG(ERROR) << "Cannot find address for service " << m_service << "!!";
                return false;
            }

            int fd = lu::platform::NULL_FD;
            struct sockaddr_storage localAddr;

            for (struct addrinfo *addr = servAddr; addr != NULL; addr = addr->ai_next)
            {
                LOG(INFO) << "ServerSocket try protocolFamily:" << addr->ai_family 
                    << ", socketType:" << addr->ai_socktype << ", protocol:" << addr->ai_protocol;

                fd = ::socket(servAddr->ai_family, servAddr->ai_socktype, servAddr->ai_protocol);

                if (fd < 0)
                {
                    LOG(WARNING) << "can not open socket!";
                    continue;
                }

                m_baseSocket = BaseSocket(fd);

                if (m_reuseAddAndPort)
                {
                    m_baseSocket.setReuseAddAndPort();
                }

                if (::bind(fd, servAddr->ai_addr, servAddr->ai_addrlen) == 0)
                {
                    LOG(INFO) << "Binding successful";

                    if (::listen(fd, numberOfConnectionInWaitQueue) == 0)
                    {
                        socklen_t addrSize = sizeof(localAddr);
                    
                        if (::getsockname(fd, (struct sockaddr *) &localAddr, &addrSize) < 0)
                        {
                            LOG(ERROR) << "getsockname failed!!";
                            return false;
                        }

                        break;
                    }
                }

                LOG(WARNING) << "Error code " << errno;

                ::close(fd);
                fd = lu::platform::NULL_FD;
            }

            if (fd == lu::platform::NULL_FD)
            {
                LOG(ERROR) << "Cannot start service " << m_service << "!!";
                ::freeaddrinfo(servAddr);
                return false;
            }

            m_baseSocket.setAddress((struct sockaddr&) localAddr);
            LOG(INFO) << "Service started " << m_baseSocket.getIP() << ":" << m_baseSocket.getPort();
            return true;
        }         

        
        BaseSocket* acceptDataSocket()
        {
            if (m_baseSocket.getFD() == nullptr)
            {
                return nullptr;
            }

            m_nativeHandle = pthread_self();


            struct sockaddr_storage clntAddr;
            socklen_t clntAddrLen = sizeof(clntAddr);
            int fd = ::accept(m_baseSocket.getFD(), (struct sockaddr *) &clntAddr, &clntAddrLen);

            if (fd < 0)
            {
                if (errno != EAGAIN)
                {
                    LOG(ERROR) << "Accept failed!!";
                }
                
                return nullptr;
            }

            return new BaseSocket(fd, (struct sockaddr &) clntAddr);
        }

        const std::string& getService() const { return m_service; }
        BaseSocket& getBaseSocket() { return m_baseSocket; }
        const lu::platform::FileDescriptor& getFD() const override final { return m_baseSocket.getFD(); }

        void stop()
        {
            if (m_baseSocket.getFD() == nullptr)
            {
                return;
            }

            LOG(INFO) << "Stop service " << m_baseSocket.getIP() << ":" << m_baseSocket.getPort();
            pthread_kill(m_nativeHandle, SIGUSR1);
        }
                    
    private:
        void onEvent(struct ::epoll_event& event) override final
        {
            if ((event.events & EPOLLHUP || event.events & EPOLLERR))
            {
                m_baseSocket = BaseSocket();
                return;
            }
            else if (!(event.events & EPOLLIN))
            {
                return;
            }
            
            for (;;)
            {
                auto dataSocket = acceptDataSocket();

                if (dataSocket == nullptr)
                {
                    break;
                }
                
                m_serverSocketCallback.onNewConnection(dataSocket);
            }
        }

        BaseSocket m_baseSocket;
        ServerSocketCallback& m_serverSocketCallback;
        std::string m_service{};        
        bool m_reuseAddAndPort{};
        pthread_t m_nativeHandle;
    };
}