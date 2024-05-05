#pragma once
#include  <platform/socket/DataSocket.h>
#include <platform/socket/SSLSocket.h>
#include <common/TemplateConstraints.h>

#include <sys/types.h>

#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>

#include <glog/logging.h>

// TODO Revisit and change the callbacks
namespace lu::platform::socket
{
    template<lu::common::NonPtrClassOrStruct DataSocketCallback, lu::common::NonPtrClassOrStruct DataSocketType>
    class ConnectSocket 
    {
    public:
        ConnectSocket(const ConnectSocket&)               = delete;
        ConnectSocket& operator=(const ConnectSocket&)    = delete;

        ConnectSocket(const std::string &host, const std::string &service)
            : m_dataSocket(nullptr),
              m_host(host),
              m_service(service)
        {
        }

        ConnectSocket(ConnectSocket &&other) noexcept : m_dataSocket(std::move(other.m_dataSocket)),
                                                        m_host(std::move(other.m_host)),
                                                        m_service(std::move(other.m_service))
        {
            other.m_dataSocket = nullptr;
        }

        ConnectSocket &operator=(ConnectSocket &&other) noexcept
        {
            m_dataSocket = std::move(other.m_dataSocket);
            m_host = std::move(other.m_host);
            m_service = std::move(other.m_service);
            other.m_dataSocket = nullptr;
            return *this;
        }
        ~ConnectSocket() {}

        bool connectToTCP(DataSocketCallback &dataSocketCallback, ::SSL_CTX* sslCtx = nullptr)
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
                LOG(INFO) << "Try connection to protocolFamily:" << addr->ai_family
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

            if (fd == lu::platform::NULL_FD)
            {
                ::freeaddrinfo(servAddr);
                LOG(ERROR) << "Can not create connection to " << m_host << ":" << m_service;
                return false;
            }

            if constexpr (std::is_same_v<decltype(std::declval<DataSocketType>().getSocket()), lu::platform::socket::SSLSocket&>)
            {
                ::SSL *ssl = ::SSL_new(sslCtx);

                if (ssl == nullptr)
                {
                    LOG(ERROR) << "Error performing TLS handshake [" << m_host << ":" << m_service << "]";
                    ::close(fd);
                    return false;
                }

                ::SSL_set_fd(ssl, fd);

                if (::SSL_set_tlsext_host_name(ssl, m_host.c_str()) != 1)
                {
                    LOG(ERROR) << "SSL cannot set hot name [" <<m_host << ":" << m_service << "]";
                    ::ERR_print_errors_fp(stderr);
                    ::close(fd);
                    ::SSL_free(ssl);
                    return false;
                }

                if (::SSL_connect(ssl) <= 0)
                {
                    LOG(ERROR) << "Error performing TLS handshake [" <<m_host << ":" << m_service << "]";
                    ::ERR_print_errors_fp(stderr);
                    ::close(fd);
                    ::SSL_free(ssl);
                    return false;
                }

                m_dataSocket.reset(new DataSocketType(dataSocketCallback, SSLSocket(ssl, fd, *connectAddr->ai_addr)));
            }
            else
            {
                m_dataSocket.reset(new DataSocketType(dataSocketCallback, BaseSocket(fd, *connectAddr->ai_addr)));
            }

            
            DLOG(INFO) << "Connecting to " << m_dataSocket->getIP() << ":" << m_dataSocket->getPort();
            ::freeaddrinfo(servAddr);
            return true;
        }

        const std::string& getHost() const { return m_host;}
        const std::string& getService() const { return m_service; }
        BaseSocket* getBaseSocket() { return m_dataSocket == nullptr ? nullptr : &m_dataSocket->getBaseSocket(); }
        std::unique_ptr<DataSocketType>& getDataSocket() { return m_dataSocket; }
        bool Receive() { return m_dataSocket->Receive(); }
        int sendMsg(void *buffer, ssize_t size) { return m_dataSocket->sendMsg(buffer, size); }

    private:
        std::unique_ptr<DataSocketType> m_dataSocket = nullptr;
        std::string m_host{};
        std::string m_service{};
    };
}