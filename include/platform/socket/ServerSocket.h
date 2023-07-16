#pragma once
#include "BaseSocket.h"
#include "DataSocket.h"
#include <platform/IFDEventHandler.h>

namespace lu::platform::socket
{
    template<lu::common::NonPtrClassOrStruct ConnectionHandler>
    class ServerSocket : public lu::platform::IFDEventHandler
    {
    public:
        ServerSocket(const ServerSocket&)               = delete;
        ServerSocket& operator=(const ServerSocket&)    = delete;

        ServerSocket(const std::string& service, ConnectionHandler &connectionHandler, bool reuseAddAndPort = true);
        ServerSocket(ServerSocket&& other) noexcept;
        ServerSocket& operator=(ServerSocket&& other) noexcept;

        bool setUpTCP(int numberOfConnectionInWaitQueue);
        
        BaseSocket* acceptDataSocket();
        const std::string& getService() const { return m_service; }
            
    private:
        void onEvent(struct ::epoll_event& event) override final;
        int getFD() const override final { return m_baseSocket.getFD(); }

        BaseSocket m_baseSocket;
        ConnectionHandler& m_connectionHandler;
        std::string m_service{};        
        bool m_reuseAddAndPort{};
    };
}