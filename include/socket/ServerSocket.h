#pragma once
#include "BaseSocket.h"
#include "DataSocket.h"

namespace lu::socket
{
    class ServerSocket : public BaseSocket
    {
    public:
        ServerSocket(const std::string& service, int numberOfConnectionQueued = 10);
        bool setUpTCP();
        
        DataSocket* acceptDataSocket();
        const std::string& getService() const { return m_service; }
            
    private:
        std::string m_service{};
        int m_numberOfConnectionQueued{};
    };
}