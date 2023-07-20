#pragma once

#include "BaseSocket.h"
#include <common/TemplateConstraints.h>
#include <platform/IFDEventHandler.h>
#include <string>

class Person;
struct sockaddr;

namespace lu::platform::socket
{
    template<lu::common::NonPtrClassOrStruct DataHandler>
    class DataSocket : public lu::platform::IFDEventHandler
    {
    public:
        DataSocket(const DataSocket&)               = delete;
        DataSocket& operator=(const DataSocket&)    = delete;

        DataSocket(BaseSocket&& baseSocket, DataHandler& dataHandler);
        DataSocket(DataSocket&& other) noexcept;
        DataSocket& operator=(DataSocket&& other) noexcept;
        virtual ~DataSocket(){}

        bool Receive();
        int sendMsg(void* buffer, ssize_t size);
        int sendFile(int fileDescriptor, int size);
        BaseSocket& getBaseSocket() { return m_baseSocket; }
        const std::string& getIP() const { return m_baseSocket.getIP(); }
        int getPort() const { return m_baseSocket.getPort(); }

    private:
        inline void readMessages();
        inline void updateForDataRead(std::size_t size);
        void onEvent(struct ::epoll_event& event) override final;
        const lu::platform::FileDescriptor& getFD() const override final { return m_baseSocket.getFD(); }
    
    protected:
        BaseSocket m_baseSocket;
        DataHandler& m_dataHandler;
        std::size_t m_headerSize{};
        std::size_t m_receiveBufferShiftSize{};
        std::size_t m_numberOfBytesInBuffer{};
        std::size_t m_readOffset{};
        std::size_t m_numberOfBytesLeftToRead{};
        std::size_t m_numberOfBytesLeftToRecv{};
    };
}
