#pragma once

#include <array>
#include <string>
#include <cstring>

namespace lu::common
{
    template <std::size_t N>
    class FixedString
    {
    public:
        FixedString():m_buffer{}{}
        FixedString(const std::string& str):m_buffer{} { str.copy(m_buffer.data(), N); }
        FixedString(char c):m_buffer{} { m_buffer[0] = c; }
        FixedString(const char* str):m_buffer{} { std::strncpy(m_buffer.data(), str, N); }
        
        void operator=(const std::string& str) { str.copy(m_buffer.data(), N);  }
        void operator=(const char* str) {  std::strncpy(m_buffer.data(), str, N); }
        bool operator==(const FixedString<N>& other) const { return m_buffer ==  other.m_buffer; }
        bool operator==(const std::string& other) const { return other.size() <= N && other.compare(0, N, m_buffer.data()) == 0; }
        bool operator==(const char* other) const { return std::strncmp(other, m_buffer.data(), N+1) == 0 ; }
        bool operator<(const FixedString<N>& other) const { return m_buffer <  other.m_buffer; }

        operator std::string() const { return std::string(m_buffer.data()); }
        auto size() const { return N; }
        auto getCString() const { return m_buffer.data(); }
        std::size_t hash() const { return std::hash<std::string_view>{}(std::string_view(m_buffer.data(), N+1));}

    private:
        std::array<char, N+1> m_buffer{};
    };
}

namespace std
{
    template <std::size_t N>
    struct hash<lu::common::FixedString<N>>
    {
        std::size_t operator()(const lu::common::FixedString<N> &fixedString) const
        {
            return fixedString.hash();
        }
    };
}