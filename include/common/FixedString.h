#pragma once

#include <array>
#include <string>
#include <cstring>
#include <ostream>
#include <cassert>

namespace lu::common
{
    template <std::size_t N>
    class FixedString
    {
    public:
        FixedString():m_buffer{}{}
        FixedString(const std::string& str) { str.copy(m_buffer.data(), N); m_buffer[N] = 0; }
        FixedString(char c) {  m_buffer[0] = c; m_buffer[1] = 0; }
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wstringop-truncation"
        FixedString(const char* str) {  std::strncpy(m_buffer.data(), str, N);  m_buffer[N] = 0; }
        FixedString(const char* str, std::size_t size)
        {  
            assert(size <= N);
            std::strncpy(m_buffer.data(), str, size); 
            m_buffer[size] = 0; 
        }

        #pragma GCC diagnostic pop
        
        void operator=(const std::string& str) { str.copy(m_buffer.data(), N);  }
        void operator=(const char* str) {  std::strncpy(m_buffer.data(), str, N); }
        bool operator==(const FixedString<N>& other) const { return m_buffer ==  other.m_buffer; }
        bool operator==(const std::string& other) const { return other.size() <= N && other.compare(0, N, m_buffer.data()) == 0; }
        bool operator==(const char* other) const { return std::strncmp(other, m_buffer.data(), N+1) == 0 ; }
        bool operator<(const FixedString<N>& other) const { return m_buffer <  other.m_buffer; }

        void append(char c, std::size_t position) 
        {
            assert(position <= N);
            m_buffer[position] = c;
            m_buffer[position+1] = '\0';
        }

        void append(const char* src, std::size_t startOffset, std::size_t size) 
        {
            assert(startOffset < N);
            assert(startOffset + size <= N);
            std::strncpy(m_buffer.data() + startOffset, src, size);
            m_buffer[startOffset + size] = '\0';
        }

        operator std::string() const { return std::string(m_buffer.data()); }
        auto size() const { return N; }
        auto getCString() const { return m_buffer.data(); }
        auto data() { return m_buffer.data(); }
        bool empty() const { return m_buffer[0] == '\0'; }
        std::size_t hash() const { return std::hash<std::string_view>{}(std::string_view(m_buffer.data(), N+1));}

        friend  std::ostream& operator<<(std::ostream& os,  const FixedString<N>& fixedString)
        {
            os << fixedString.getCString();
            return os;
        }

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