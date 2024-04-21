#pragma once
#include<platform/FileDescriptor.h>

#include<string>
#include<memory>

#include <sys/socket.h>

struct timeval;
struct sockaddr;

namespace lu::platform::socket
{
    enum ShutSide
    {
        ShutdownRead = SHUT_RD,
        ShutdownWrite = SHUT_WR,
        ShutdownReadWrite = SHUT_RDWR
    };

    class BaseSocket
    {
    public:
        BaseSocket(const BaseSocket&)               = delete;
        BaseSocket& operator=(const BaseSocket&)    = delete;

        BaseSocket(int fileDescriptor = lu::platform::NULL_FD);
        BaseSocket(int fileDescriptor, const sockaddr& address);
        ~BaseSocket() = default;

        BaseSocket(BaseSocket&& other) noexcept;
        BaseSocket& operator=(BaseSocket&& other) noexcept;
        
        const lu::platform::FileDescriptor& getFD() const;
        int getRxBufferSize() const;
        int getTxBufferSize() const;
        const std::string& getIP() const { return m_ip; }
        int getPort() const { return m_port; }
        template<typename T>
        T getSocketOption(int level, int option) const;

        void setAddress(const sockaddr& address);
        void setNonBlocking();
        void setBlocking();
        bool setRxBufferSize(int size);
        bool setTxBufferSize(int size);
        void setReuseAddAndPort(bool reuseAddAndPort) { m_reuseAddAndPort = reuseAddAndPort; }
        bool setTCPMaxSendRate(unsigned long long int rateInBitPerSec);
        bool setTCPNoDelay();
        bool setKeepAlive();
        bool setMinimumDataToReturnRecv(int numberOfBytes);
        bool setMinimumDataToReturnSend(int numberOfBytes);
        bool setReceiveTimeOut(const struct timeval &timeout);
        bool setSendTimeOut(const struct timeval &timeout);
        bool setDataFlushTimeoutOnClose(int waitTimeInSec);
        bool setTCPKeepAlive(int maxIdleTime, int interval, int numberOfTry);
        bool setBufferTCPSendData();
        bool setMaxSendDataWaitThreshold(int numberOfBytes);
        bool setReuseAddAndPort();
        
        template<typename T>
        bool setSocketOption(int level, int option, const T& value);
        int stop(ShutSide);
        int close();

        bool readDataSocket(uint8_t *buf, size_t size, ssize_t &readCount);
        bool readDataSocket(struct iovec* dataBufferVec, int numOfVBuffers, ssize_t &readCount);

        ssize_t send(void *buffer, ssize_t size);
        ssize_t sendfile( int fileDescriptor, ssize_t size);

    protected:
        

        
        void getIPAndPort(const struct sockaddr &address);

    protected:
        std::unique_ptr<lu::platform::FileDescriptor> m_fd;
        std::string m_ip{};
        int m_port{};

    private:
        int m_socketFlags = 0;
        int m_socketDescriptorFlags = 0;
        bool m_reuseAddAndPort = true;
    };
}