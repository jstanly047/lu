#include <platform/socket/SSLSocket.h>

using namespace lu::platform::socket;

SSLSocket::SSLSocket(::SSL* ssl, int fileDescriptor) : BaseSocket(fileDescriptor), m_ssl(ssl)
{

}

SSLSocket::SSLSocket(::SSL* ssl, int fileDescriptor, const sockaddr &address) : BaseSocket(fileDescriptor, address), m_ssl(ssl)
{

}

SSLSocket::SSLSocket(::SSL *ssl, BaseSocket &&baseSocket) : BaseSocket(std::move(baseSocket)), m_ssl(ssl)
{
}

SSLSocket::SSLSocket(SSLSocket &&other) noexcept : BaseSocket(std::move(other))
{
    std::swap(m_ssl, other.m_ssl);
}

SSLSocket &SSLSocket::operator=(SSLSocket &&other) noexcept
{
    BaseSocket::operator=(std::move(other));
    std::swap(m_ssl, other.m_ssl);
    return *this;
}

int SSLSocket::stop([[maybe_unused]]ShutSide)
{
    return ::SSL_shutdown(m_ssl);
}

int SSLSocket::close()
{
    ::SSL_shutdown(m_ssl);
    ::SSL_free(m_ssl);
    m_ssl = nullptr;
    return BaseSocket::close();
}

SSLSocket::~SSLSocket()
{
    if (m_ssl != nullptr)
    {
        ::SSL_shutdown(m_ssl);
        ::SSL_free(m_ssl);
    }
}

bool SSLSocket::readDataSocket(uint8_t *buf, int size, ssize_t &readCount)
{
    readCount = ::SSL_read(m_ssl, buf, size);

    if (readCount < 0)
    {
        int sslError = ::SSL_get_error(m_ssl, readCount);
        readCount = 0;
        return (sslError == SSL_ERROR_WANT_READ || sslError == SSL_ERROR_WANT_WRITE);
    }

    return readCount != 0;
}

ssize_t SSLSocket::send(void *buffer, int size)
{
    if (m_ssl == nullptr)
    {
        return 0;
    }

    ssize_t totalSent = 0;
    auto uint8_tBuffer = reinterpret_cast<uint8_t *>(buffer);

    while (totalSent < size)
    {
        // TODO we can try replace this by ::writev (scatter/gather IO)
        int numBytesSend = ::SSL_write(m_ssl, uint8_tBuffer, size);

        if (numBytesSend < 0)
        {
            int sslError = ::SSL_get_error(m_ssl, numBytesSend);
            
            if (sslError == SSL_ERROR_WANT_READ || sslError == SSL_ERROR_WANT_WRITE)
            {
                continue;
            }

            return totalSent;
        }
        else if (numBytesSend != size)
        {
            int sslError = ::SSL_get_error(m_ssl, numBytesSend);

            if (sslError == SSL_ERROR_WANT_READ || sslError == SSL_ERROR_WANT_WRITE)
            {
                continue;
            }

            return totalSent;
        }

        return numBytesSend;
    }

    return totalSent;
}
