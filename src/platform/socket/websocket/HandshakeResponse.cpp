#include <platform/socket/websocket/HandshakeResponse.h>
#include <platform/socket/websocket/HandshakeRequest.h>
#include <crypto/Hash.h>

using namespace lu::platform::socket::websocket;

namespace
{
    std::vector<int> listIntersection(std::vector<int> list1, std::vector<int> list2)
    {
        std::vector<int> result;
        std::sort(list1.begin(), list1.end(), std::greater<int>());
        std::sort(list2.begin(), list2.end(), std::greater<int>());
        std::set_intersection(list1.cbegin(), list1.cend(),
                              list2.cbegin(), list2.cend(),
                              std::back_inserter(result), std::greater<int>());
        return result;
    }
}

HandshakeResponse::HandshakeResponse(const HandshakeRequest &handshakeRequest, const std::vector<int>& supportedVersions, const std::string& supportedProtocol)
{
    const std::string protocol = [&] {
                const auto it = std::find(
                            handshakeRequest.getProtocols().begin(), handshakeRequest.getProtocols().end(),
                            supportedProtocol);

                return it == handshakeRequest.getProtocols().end() ? "" : *it;
            }();

    //****** This not standard Websocket Start******
    if (handshakeRequest.getUpgrade() == "tcp")
    {
        m_response = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: tcp\r\n\r\n";
        return;
    }
    //****** This not standard Websocket End******

    const std::vector<int> matchingVersions = listIntersection(supportedVersions, handshakeRequest.getWebsocketVersions());

    if (protocol.empty() || matchingVersions.empty())
    {
        return;
    }

    m_response =    "HTTP/1.1 101 Switching Protocols\r\n"
                    "Upgrade: websocket\r\n"
                    "Connection: Upgrade\r\n"
                    "Sec-WebSocket-Accept: ";
    m_response += getAcceptKey(handshakeRequest.getKey());
    m_response += "\r\n";
    m_response += "Sec-WebSocket-Protocol: " + supportedProtocol;
    m_response += "\r\n\r\n";
}

std::string HandshakeResponse::getAcceptKey(const std::string &key)
{
    const std::string tempKey = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    // TODO: thread local hash instance
    lu::crypto::Hash<lu::crypto::HashAlgo::SHA> hash;
    hash.init();
    return hash.getBase64Hash(tempKey);
}
