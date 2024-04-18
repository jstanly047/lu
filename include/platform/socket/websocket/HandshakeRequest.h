#pragma once
#include <platform/socket/BaseSocket.h>
#include <string>
#include <vector>
#include <map>

namespace lu::platform::socket::websocket
{
    struct InitialRequestInfo
    {
        std::string resourceName;
        std::string host;
        std::string port;
        std::string origin;
        std::string wsVersion;
        std::string extensions;
        std::vector<std::string> protocols;
        std::vector<std::pair<std::string, std::string>> headers;
    };
    
    class HandshakeRequest
    {
    public:
        HandshakeRequest(const HandshakeRequest&)               = delete;
        HandshakeRequest& operator=(const HandshakeRequest&)    = delete;
        HandshakeRequest(HandshakeRequest&& other)              = delete;
        HandshakeRequest& operator=(HandshakeRequest&& other)   = delete;
        
        HandshakeRequest(const lu::platform::socket::BaseSocket& baseSocket);
        bool readServerRequest(const char* request, unsigned int length, const char* expectedResource);
        static std::string createHandShakeRequest(const InitialRequestInfo& handshakeInfo, const std::string& key);
        static std::string generateKey();

        auto& getKey() const { return m_key; }

        auto& getProtocols() const { return m_protocols; }
        auto& getWebsocketVersions() const { return m_websocketVersions; }
        

    private:
        const BaseSocket& m_baseSocket;
        std::vector<std::string> m_protocols;
        std::vector<int> m_websocketVersions;
        std::string m_key;
        std::map<std::string, std::string> m_keyValue;
    };
}