#pragma once
#include<platform/FileDescriptor.h>

#include<string>
#include<memory>

struct timeval;
struct sockaddr;

namespace lu::platform::socket
{
    class BaseSocket
    {
    public:
        BaseSocket(const BaseSocket&)               = delete;
        BaseSocket& operator=(const BaseSocket&)    = delete;

        BaseSocket(int socketId = lu::platform::NULL_FD);
        BaseSocket(int socketId, const sockaddr& address);
        ~BaseSocket();

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
        bool setSocketDescriptorFlags();

        template<typename T>
        bool setSocketOption(int level, int option, const T& value);
        void stop();

        
        //TODO close function for DataSocket to close the baseSocket

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