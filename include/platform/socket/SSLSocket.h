#pragma once
#include <platform/socket/BaseSocket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace lu::platform::socket
{
    class SSLSocket : public BaseSocket
    {
    public:
        SSLSocket(const SSLSocket&)               = delete;
        SSLSocket& operator=(const SSLSocket&)    = delete;

        SSLSocket(::SSL* ssl = nullptr, int fileDescriptor = lu::platform::NULL_FD);
        SSLSocket(::SSL* ssl, int fileDescriptor, const sockaddr& address);
        SSLSocket(::SSL* ssl, BaseSocket&& baseSocket);
        ~SSLSocket();

        SSLSocket(SSLSocket&& other) noexcept;
        SSLSocket& operator=(SSLSocket&& other) noexcept;
        int stop(ShutSide);
        int close();

        bool readDataSocket(uint8_t *buf, int size, ssize_t &readCount);
        ssize_t send(void *buffer, int size);

    private:
        ::SSL* m_ssl{};
    };
}