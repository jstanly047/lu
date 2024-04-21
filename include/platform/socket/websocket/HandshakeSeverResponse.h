#pragma once

#include <platform/socket/BaseSocket.h>
#include <string>
#include <vector>
#include <map>

namespace lu::platform::socket::websocket
{
    class HandshakeSeverResponse
    {
    public:
        HandshakeSeverResponse(const lu::platform::socket::BaseSocket& baseSocket);
        bool readServerResponse(const char* request, unsigned int length, const std::string& key);
        const std::string& getUpgrade() const;

    private:
        const BaseSocket& m_baseSocket;
        std::vector<std::string> m_protocols;
        std::map<std::string, std::string> m_keyValue;
    };
}