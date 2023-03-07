#pragma once

struct timeval;

namespace lu::socket
{
    constexpr static int NULL_SOCKET = -1;
    constexpr int getDefaultTCPBlockSize() { return 128 * 1024; }

    class BaseSocket
    {
    public:
        BaseSocket(BaseSocket&& move) noexcept;
        BaseSocket& operator=(BaseSocket&& move) noexcept;
        BaseSocket(const BaseSocket&)               = delete;
        BaseSocket& operator=(const BaseSocket&)    = delete;
        ~BaseSocket();
        void setNonBlocking();
        bool registerEpoll(int epollFd);
        bool setRecvBufferSize(int size);
        bool setSendBufferSize(int size);
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

        int getRcvBufferSize() const;
        int getSendBufferSize() const;

    protected:
        BaseSocket(int socketId);
        bool setReuseAddAndPort();
        bool setSocketDescriptorFlags();

        int m_socketId;
        int m_socketFlags = 0;
        int m_socketDescriptorFlags = 0;
        bool m_reuseAddAndPort = true;
    };
}