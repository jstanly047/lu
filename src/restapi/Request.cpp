#include <restapi/Request.h>

using namespace snap::restapi;

Request::Request(Method method) : m_method(method)
{
}

void Request::append(const std::string &key, const std::string &value)
{
    if (m_data.empty())
    {
        m_data += key + "=" + value;
        return;
    }

    m_data += "&" + key + "=" + value;
}

void Request::addHttpHeader(const std::string& key, const std::string& value)
{
    m_extraHttpHeader.emplace_back(key + ": " + value);
}
