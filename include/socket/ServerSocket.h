#pragma once
#include "BaseSocket.h"
#include "DataSocket.h"

namespace lu::socket
{
    constexpr int MAX_PENDING = 10;

    class ServerSocket : public BaseSocket
    {
    public:
        ServerSocket(const std::string& service);
        bool setUpTCP();
        
        DataSocket* acceptDataSocket();
        const std::string& getService() const { return m_service; }
            
    private:
        std::string m_service;
    };
}