#pragma once
#include  <platform/socket/DataSocket.h>
#include <common/TemplateConstraints.h>

namespace lu::platform::socket
{
    template<lu::common::NonPtrClassOrStruct DataHandler>
    class ConnectSocket 
    {
    public:
        ConnectSocket(const ConnectSocket&)               = delete;
        ConnectSocket& operator=(const ConnectSocket&)    = delete;

        ConnectSocket(const std::string& host, const std::string& service);
        ConnectSocket(ConnectSocket&& other) noexcept;
        ConnectSocket& operator=(ConnectSocket&& other) noexcept;

        bool connectToTCP(DataHandler& dataHandler);

        const std::string& getHost() const { return m_host;}
        const std::string& getService() const { return m_service; }
        BaseSocket* getBaseSocket() { return m_dataSocket == nullptr ? nullptr : &m_dataSocket->getBaseSocket(); }

    private:
        DataSocket<DataHandler> *m_dataSocket = nullptr;
        std::string m_host{};
        std::string m_service{};
    };
}