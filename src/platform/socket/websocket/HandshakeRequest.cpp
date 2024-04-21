#include <platform/socket/websocket/HandshakeRequest.h>
#include <platform/socket/websocket/HandshakeResponse.h>
#include <utils/DelimiterTextParser.h>
#include <crypto/Base64EncodeDecode.h>
#include <utils/Utils.h>
#include <string_view>
#include <sstream>
#include <random>

#include <glog/logging.h>

using namespace lu::platform::socket::websocket;

HandshakeRequest::HandshakeRequest(const lu::platform::socket::BaseSocket& baseSocket):
    m_baseSocket(baseSocket)
{
}

std::string HandshakeRequest::getResponse(std::string_view& dataStringView, const std::string& expectedResource, 
    const std::string& supportedProtocol, const std::vector<int>& supportedVersion)
{
    unsigned int readHeader = 0U;
    DLOG(INFO) << "Request [" << dataStringView << "] from [" << m_baseSocket.getIP() << "]";

    for (;;)
    {
        auto location = dataStringView.find("\r\n", readHeader);

        if (location == readHeader) // End of header
        {
            readHeader += 2U;
            break;
        }

        std::string_view line(dataStringView.data() + readHeader, location - readHeader);

        if (m_keyValue.empty())
        {
            try
            {
                static char requestDelimiter = ' ';
                lu::utils::DelimiterTextParser requestParser(line, requestDelimiter);
                const std::string verb(requestParser.next());
                const std::string resource(requestParser.next());
                const std::string httpProtocol(requestParser.next());

                static char httpVersionDelimiter = '/';
                lu::utils::DelimiterTextParser httpProtocolParser(httpProtocol, httpVersionDelimiter);
                const std::string protocol(httpProtocolParser.next());
                const float httpVersion = httpProtocolParser.nextFloat();
                m_keyValue.emplace("request-method", line);

                if (verb != "GET" || resource != std::string(expectedResource) || httpVersion < 1.1F)
                {
                    LOG(ERROR) << "Invalid request [" << line << "] from [" << m_baseSocket.getIP() << "]";
                    return std::string();
                }

                readHeader = location + 2u;
                continue;
            }
            catch (const std::exception &e)
            {
                return  std::string();
            }
        }

        auto delimiterPos = line.find(":");

        if (delimiterPos == std::string::npos)
        {
            LOG(ERROR) << "Incorrect line [" << line << "] from [" << m_baseSocket.getIP() << "]";
            return  std::string();
        }

        std::string key(line.substr(0, delimiterPos));
        lu::utils::Utils::toLower(key);
        m_keyValue.emplace(key, line.substr(delimiterPos + 2));
        readHeader = location + 2u;   
    }

    auto itr = m_keyValue.find("upgrade");

    if (itr == m_keyValue.end())
    {
        LOG(ERROR) << "Incorrect 'Upgrade'!! Request [" << dataStringView << "] from [" << m_baseSocket.getIP() << "]";
        return  std::string();
    }

    m_upgrade = itr->second;

    //****** This not standard Websocket Start******
    if (itr->second == "tcp")
    {
        HandshakeResponse response(*this, supportedVersion, supportedProtocol);
        return  std::move(response.getResponse());
    }

    //****** This not standard Websocket End******
    if (itr->second != "websocket")
    {
        LOG(ERROR) << "Incorrect 'Upgrade'!! Request [" << dataStringView << "] from [" << m_baseSocket.getIP() << "]";
        return  std::string();
    }

    itr = m_keyValue.find("sec-websocket-key");

    if (itr == m_keyValue.end())
    {
        LOG(ERROR) << "Sec-Websocket-Key missing!! Request [" << dataStringView << "] from [" << m_baseSocket.getIP() << "]";
        return  std::string();
    }

    m_key = itr->second;

    
    itr = m_keyValue.find("connection");

    if (itr == m_keyValue.end() || lu::utils::Utils::toLower(itr->second) != "upgrade")
    {
        LOG(ERROR) << "Incorrect 'Connection'!! Request [" << dataStringView << "] from [" << m_baseSocket.getIP() << "]";
        return  std::string();
    }

    itr = m_keyValue.find("sec-websocket-protocol");

    if (itr == m_keyValue.end() || itr->second.empty())
    {
        LOG(ERROR) << "Sec-Websocket-Protocol missing!! Request [" << dataStringView << "] from [" << m_baseSocket.getIP() << "]";
        return  std::string();
    }

    static std::string valueDelimeter = ", ";
    lu::utils::DelimiterTextParser protocolsParse(itr->second, valueDelimeter);
    auto value = protocolsParse.next();

    try
    {
        while (value.empty() == false)
        {
            m_protocols.emplace_back(value);
            value = protocolsParse.next();
        };
    }
    catch(const std::exception& e)
    {
        
    }

    itr = m_keyValue.find("sec-websocket-version");

    if (itr == m_keyValue.end())
    {
        LOG(ERROR) << "Sec-WebSocket-Version missing!! Request [" << dataStringView << "] from [" << m_baseSocket.getIP() << "]";
        return  std::string();
    }

    protocolsParse.nextLine(itr->second);
    auto version = protocolsParse.nextInt();

    try
    {
        while (version != 0)
        {
            m_websocketVersions.push_back(version);
            version = protocolsParse.nextInt();
        };
    }
    catch(const std::exception& e)
    {
        
    }
    
    if (m_protocols.empty())
    {
        LOG(ERROR) << "Incorrect 'Sec-Websocket-Protocol'!! Request [" << dataStringView << "] from [" << m_baseSocket.getIP() << "]";
        return  std::string();
    }

    HandshakeResponse response(*this, supportedVersion, supportedProtocol);

    return std::move(response.getResponse());
}

std::string HandshakeRequest::createHandShakeRequest(const InitialRequestInfo& handshakeInfo, const std::string& key)
{
    std::stringstream handshakeRequest;
    handshakeRequest << "GET " << handshakeInfo.resourceName << " HTTP/1.1\r\n" <<
                        "Host: " <<  handshakeInfo.host << "\r\n";
                       
    
    if (handshakeInfo.upgrade.empty())
    {
         handshakeRequest << "Upgrade: websocket\r\n";
    }
    else
    {
         handshakeRequest << "Upgrade: " <<  handshakeInfo.upgrade << "\r\n";
    }

    handshakeRequest << "Connection: Upgrade\r\nSec-WebSocket-Key: " << key << "\r\n";

    if (!handshakeInfo.origin.empty())
    {
        handshakeRequest << "Origin: " << handshakeInfo.origin << "\r\n";
    }

    handshakeRequest << "Sec-WebSocket-Version: " << handshakeInfo.wsVersion << "\r\n";

    if (handshakeInfo.extensions.empty() == false)
    {
        handshakeRequest << "Sec-WebSocket-Extensions: " << handshakeInfo.extensions << "\r\n";
    }

    if (handshakeInfo.protocols.empty() == false) 
    {
        handshakeRequest << "Sec-WebSocket-Protocol: " << handshakeInfo.protocols[0];

        for (unsigned int i = 1; i < handshakeInfo.protocols.size(); i++)
        {
            handshakeRequest << ", " << handshakeInfo.protocols[i];
        }

        handshakeRequest << "\r\n";
    }

    for (const auto &header : handshakeInfo.headers)
    {
        handshakeRequest << header.first << ": " << header.second << "\r\n";
    }

    handshakeRequest << "\r\n";

    return handshakeRequest.str();
}

std::string HandshakeRequest::generateKey()
{
    thread_local std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis;
    dis(gen);

    lu::crypto::DataWrap dataWrap(4 * sizeof(uint32_t));

    for (int i = 0; i < 4; ++i) 
    {
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> dis;
        uint32_t value = dis(gen);
        dataWrap.append(static_cast<const char *>(static_cast<const void *>(&value)), sizeof(uint32_t));
    }

    return lu::crypto::Base64EncodeDecode::encode(dataWrap);
}
