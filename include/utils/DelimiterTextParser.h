#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <ctime>

namespace lu::utils
{
    template<typename DelimeterType>
    class DelimiterTextParser
    {
    public:
        DelimiterTextParser(const std::string& line, const DelimeterType&  delimeter);
        void nextLine(const std::string& line);
        std::string_view next() const;
        char nextChar() const;
        int nextInt() const;
        bool nextBool() const;
        double nextDouble() const;
        float nextFloat() const;
        std::time_t nextDateTime(const std::string timeFormat="%Y%m%d%H%M%S") const;
        int getLineNumber() const { return m_lineNumber; }


    private:
        inline void logOutOfRange() const;

        const DelimeterType& m_delimeter;
        std::string_view m_line{};
        size_t m_delimeterLen{};
        mutable size_t m_startPos{};
        mutable int m_currentIndex = 1;
        int m_lineNumber = 1;
    };
}