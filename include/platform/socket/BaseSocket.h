#pragma once
#include<string>

struct timeval;
struct sockaddr;

namespace lu::platform::socket
{
    class BaseSocket
    {
    public:
        BaseSocket(const BaseSocket&)               = delete;
        BaseSocket& operator=(const BaseSocket&)    = delete;

        BaseSocket(int socketId);
        BaseSocket(int socketId, const sockaddr& address);
        ~BaseSocket();

        BaseSocket(BaseSocket&& other) noexcept;
        BaseSocket& operator=(BaseSocket&& other) noexcept;
        
        int getFD() const { return m_fd; }
        void setNonBlocking();
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

        int getRxBufferSize() const;
        int getTxBufferSize() const;

        const std::string& getIP() const { return m_ip; }
        int getPort() const { return m_port; }
        bool setReuseAddAndPort();
        bool setSocketDescriptorFlags();
        //TODO close function for DataSocket to close the baseSocket

    protected:
        

        
        void getIPAndPort(const struct sockaddr &address);

    protected:
        int m_fd;
        std::string m_ip{};
        int m_port{};

    private:
        int m_socketFlags = 0;
        int m_socketDescriptorFlags = 0;
        bool m_reuseAddAndPort = true;
    };
}