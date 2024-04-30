#include <platform/socket/websocket/HandshakeSeverResponse.h>
#include <platform/socket/websocket/HandshakeResponse.h>
#include <utils/DelimiterTextParser.h>
#include <utils/Utils.h>
#include <string_view>

#include <glog/logging.h>

using namespace lu::platform::socket::websocket;

HandshakeSeverResponse::HandshakeSeverResponse(const lu::platform::socket::BaseSocket &baseSocket) : m_baseSocket(baseSocket)
{

}

bool HandshakeSeverResponse::readServerResponse(const char *request, unsigned int length, const std::string& key)
{
    unsigned int readHeader = 0U;
    std::string_view dataStringView(request, length);
    DLOG(INFO) << "Sever response [" << dataStringView << "] from [" << m_baseSocket.getIP() << "]";

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
            try
            {
                static char requestDelimiter = ' ';
                lu::utils::DelimiterTextParser requestParser(line, requestDelimiter);
                const std::string httpProtocol(requestParser.next());
                auto statusCode = requestParser.nextInt();

                static char httpVersionDelimiter = '/';
                lu::utils::DelimiterTextParser httpProtocolParser(httpProtocol, httpVersionDelimiter);
                const std::string protocol(httpProtocolParser.next());
                const float httpVersion = httpProtocolParser.nextFloat();
                m_keyValue.emplace("ServerResponse", std::to_string(statusCode));

                if (httpVersion < 1.1F || statusCode != 101)
                {
                    LOG(ERROR) << "close response from server [" << line << "] from [" << m_baseSocket.getIP() << "]";
                    return false;
                }

                readHeader = location + 2u;
                continue;
            }
            catch (const std::exception &e)
            {
                return false;
            }
        }

        auto delimiterPos = line.find(":");

        if (delimiterPos == std::string::npos)
        {
            LOG(ERROR) << "Incorrect line [" << line << "] from [" << m_baseSocket.getIP() << "]";
            return false;
        }

        std::string key(line.substr(0, delimiterPos));
        lu::utils::Utils::toLower(key);
        m_keyValue.emplace(key, line.substr(delimiterPos + 2));
        readHeader = location + 2u;   
    }

    auto itr = m_keyValue.find("upgrade");

    if (itr == m_keyValue.end())
    {
        LOG(ERROR) << "Incorrect 'Upgrade'!! Response [" << dataStringView << "] from [" << m_baseSocket.getIP() << "]";
        return false;
    }

    //****** This not standard Websocket Start******
    if (itr->second == "tcp")
    {
        return true;
    }
    //****** This not standard Websocket end******

    if (itr->second != "websocket")
    {
        LOG(ERROR) << "Incorrect 'Upgrade'!! Response [" << dataStringView << "] from [" << m_baseSocket.getIP() << "]";
        return false;
    }

    itr = m_keyValue.find("connection");

    if (itr == m_keyValue.end() || lu::utils::Utils::toLower(itr->second) != "upgrade")
    {
        LOG(ERROR) << "Incorrect 'Connection'!! Response [" << dataStringView << "] from [" << m_baseSocket.getIP() << "]";
        return false;
    }

    itr = m_keyValue.find("sec-websocket-protocol");

    if (itr != m_keyValue.end() && itr->second.empty() == false)
    {
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
    }

    itr = m_keyValue.find("sec-websocket-accept");

    if (itr == m_keyValue.end())
    {
        LOG(ERROR) << "Sec-Websocket-Accept missing!! Request [" << dataStringView << "] from [" << m_baseSocket.getIP() << "]";
        return false;
    }

    if (itr->second != HandshakeResponse::getAcceptKey(key))
    {
        LOG(ERROR) << "Incorrect Sec-Websocket-Accept!! Request [" << dataStringView << "] from [" << m_baseSocket.getIP() << "]";
        return false;
    }

    return true;
}

const std::string& HandshakeSeverResponse::getUpgrade() const
{
    static std::string emptyString;
    auto itr = m_keyValue.find("upgrade");
    return itr == m_keyValue.end() ? emptyString : itr->second;
}
