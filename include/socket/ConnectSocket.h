#pragma once
#include "DataSocket.h"

namespace lu::socket
{
    class ConnectSocket : public DataSocket
    {
    public:
        ConnectSocket(const std::string& host, const std::string& service);
        bool connectToTCP();

        const std::string& getHost() const { return m_host;}
        const std::string& getService() const { return m_service; }

    private:
        std::string m_host;
        std::string m_service;

    };
}