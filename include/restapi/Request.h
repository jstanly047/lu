#pragma once
#include <string>
#include <vector>
#include <type_traits>

namespace snap::restapi
{
    enum struct Method : int
    {
        GET = 0,
        POST = 1,
        PUT = 2,
        DELETE = 3
    };

    class Request
    {
    public:
        Request(Method, const std::string&);
        void append(const std::string&, const std::string&);

        void append(const std::string& key, auto value)
        {
            if constexpr (std::is_same_v<decltype(value), const char*>)
            {
                append(key, std::string(value));
            }
            else
            {
                append(key, std::to_string(value));
            }
        }

        void addHttpHeader(const std::string&, const std::string&);

        void addHttpHeader(const std::string& key, auto value)
        {
            if constexpr (std::is_same_v<decltype(value), const char*>)
            {
                addHttpHeader(key, std::string(value));
            }
            else
            {
                addHttpHeader(key, std::to_string(value));
            }
        }

        void clear() { m_data.clear(); }

        Method getMethod() const { return m_method; }
        auto& getResource() const { return m_resource; }
        auto& getData() const { return m_data; }
        auto& getExtraHttpHeader() const { return m_extraHttpHeader; }


    private:
        Method m_method;
        std::string m_resource;
        std::string m_data;
        std::vector<std::string> m_extraHttpHeader;
    };
}