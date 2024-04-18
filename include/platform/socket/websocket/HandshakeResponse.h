#pragma once

#include <platform/socket/BaseSocket.h>
#include <string>
#include <vector>

namespace lu::platform::socket::websocket
{
    class HandshakeRequest;

    class HandshakeResponse
    {
    public:
        HandshakeResponse(const HandshakeResponse&)               = delete;
        HandshakeResponse& operator=(const HandshakeResponse&)    = delete;
        HandshakeResponse(HandshakeResponse&& other)              = delete;
        HandshakeResponse& operator=(HandshakeResponse&& other)   = delete;

        HandshakeResponse(const HandshakeRequest &handshakeRequest, const std::vector<int>& supportedVersions, const std::string& supportedProtocol);
        auto& getResponse() const { return m_response; }
        static std::string getAcceptKey(const std::string& key);

    private:
        std::string m_response;
    };
}