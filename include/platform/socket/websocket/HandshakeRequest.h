#pragma once
#include <platform/socket/BaseSocket.h>
#include <string>
#include <vector>
#include <map>

namespace lu::platform::socket::websocket
{
    class HandshakeRequest
    {
    public:
        HandshakeRequest(const HandshakeRequest&)               = delete;
        HandshakeRequest& operator=(const HandshakeRequest&)    = delete;
        HandshakeRequest(HandshakeRequest&& other)              = delete;
        HandshakeRequest& operator=(HandshakeRequest&& other)   = delete;
        
        HandshakeRequest(const lu::platform::socket::BaseSocket& baseSocket);
        bool readRequest(const char* request, unsigned int length, const char* expectedResource);
        auto& getKey() const { return m_key; }

        auto& getProtocols() const { return m_protocols; }
        auto&getWebsocketVersions() const { return m_websocketVersions; }
        

    private:
        const BaseSocket& m_baseSocket;
        std::vector<std::string> m_protocols;
        std::vector<int> m_websocketVersions;
        std::string m_key;
        std::map<std::string, std::string> m_keyValue;
    };
}