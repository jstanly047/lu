#include <platform/socket/websocket/HandshakeRequest.h>
#include <utils/DelimiterTextParser.h>
#include <string_view>

#include <glog/logging.h>

using namespace lu::platform::socket::websocket;

std::string& toLower(std::string &str) 
{
    for (auto& c: str)
    {
        c = std::towlower(c);
    }

    return str;
}

HandshakeRequest::HandshakeRequest(const lu::platform::socket::BaseSocket& baseSocket):
    m_baseSocket(baseSocket)
{
}

bool HandshakeRequest::readRequest(const char *request, unsigned int length, const char* expectedResource)
{
    unsigned int readHeader = 0U;
    std::string_view dataStringView(request, length);
    DLOG(INFO) << "Request [" << dataStringView << "] from [" << m_baseSocket.getIP() << "]";

    for (;;)
    {
        auto location = dataStringView.find("\r\n", readHeader);

        if (location == readHeader) // End of header
        {
            readHeader += 2U;
            break;
        }

        std::string_view line(request + readHeader, location - readHeader);

        if (m_keyValue.empty())
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
                return false;
            }

            readHeader = location + 2u;
            continue;
        }

        auto delimiterPos = line.find(":");

        if (delimiterPos == std::string::npos)
        {
            LOG(ERROR) << "Incorrect line [" << line << "] from [" << m_baseSocket.getIP() << "]";
            return false;
        }

        std::string key(line.substr(0, delimiterPos));
        toLower(key);
        m_keyValue.emplace(key, line.substr(delimiterPos + 2));
        readHeader = location + 2u;   
    }

    auto itr = m_keyValue.find("sec-websocket-key");

    if (itr == m_keyValue.end())
    {
        LOG(ERROR) << "Sec-Websocket-Key missing!! Request [" << dataStringView << "] from [" << m_baseSocket.getIP() << "]";
        return false;
    }

    m_key = itr->second;

    itr = m_keyValue.find("upgrade");

    if (itr == m_keyValue.end() || itr->second != "websocket")
    {
        LOG(ERROR) << "Incorrect 'Upgrade'!! Request [" << dataStringView << "] from [" << m_baseSocket.getIP() << "]";
        return false;
    }

    itr = m_keyValue.find("connection");

    if (itr == m_keyValue.end() || toLower(itr->second) != "upgrade")
    {
        LOG(ERROR) << "Incorrect 'Connection'!! Request [" << dataStringView << "] from [" << m_baseSocket.getIP() << "]";
        return false;
    }

    itr = m_keyValue.find("sec-websocket-protocol");

    if (itr == m_keyValue.end())
    {
        LOG(ERROR) << "Sec-Websocket-Protocol missing!! Request [" << dataStringView << "] from [" << m_baseSocket.getIP() << "]";
        return false;
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
        return false;
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
        LOG(ERROR) << "Incorrect 'sec-websocket-protocol'!! Request [" << dataStringView << "] from [" << m_baseSocket.getIP() << "]";
        return false;
    }

    /*itr = m_keyValue.find("sec-websocket-extensions");

    if (itr == m_keyValue.end())
    {
        LOG(ERROR) << "Sec-Websocket-Extensions missing!! Request [" << dataStringView << "] from [" << m_baseSocket.getIP() << "]";;
        return false;
    }*/

    return true;
}
