#include <utils/DelimiterTextParser.h>
#include <utils/Utils.h>
#include <utility>
#include <stdexcept>

using namespace lu::utils;


template <typename DelimeterType>
DelimiterTextParser<DelimeterType>::DelimiterTextParser(const std::string& line, const DelimeterType&  delimeter, int startLine):
    m_delimeter(delimeter),
    m_line(line.data()),
    m_delimeterLen(getDelimiterSize(delimeter)),
    m_lineNumber(startLine)
{
}

template <typename DelimeterType>
DelimiterTextParser<DelimeterType>::DelimiterTextParser(std::string_view& line, const DelimeterType&  delimeter, int startLine):
    m_delimeter(delimeter),
    m_line(line),
    m_delimeterLen(getDelimiterSize(delimeter)),
    m_lineNumber(startLine)
{
}

template <typename DelimeterType>
void DelimiterTextParser<DelimeterType>::nextLine(const std::string& line)
{
    m_line = std::string_view(line.data());
    m_startPos = {};
    m_currentIndex = 1;
    m_lineNumber++;
}

template <typename DelimeterType>
std::string_view DelimiterTextParser<DelimeterType>::next() const
{
    auto endPos = m_line.find(m_delimeter, m_startPos);

    if (endPos == std::string::npos)
    {
        if (m_startPos >= m_line.size())
        {
            logOutOfRange();
            return std::string_view();
        }
        
                    endPos =  m_line.size();
       
    }

    m_currentIndex++;
    auto len = endPos - m_startPos;
    auto retVal = std::string_view(m_line.data() + std::exchange(m_startPos, endPos + m_delimeterLen) , len);
    auto start = retVal.find_first_not_of(' ');

    if (start != std::string_view::npos)
    {
        retVal = retVal.substr(start);
    }

    auto end = retVal.find_last_not_of(' ');

    if (end != std::string_view::npos)
    {
        retVal = retVal.substr(0,end+1);
    }

    return retVal == "null" || retVal == "Null" || retVal == "NULL" ? std::string_view() : retVal;
}

template <typename DelimeterType>
char DelimiterTextParser<DelimeterType>::nextChar() const
{
    auto strView = next();
    return strView.empty() ? ' ' : strView.at(0);
}

template <typename DelimeterType>
int DelimiterTextParser<DelimeterType>::nextInt() const
{
    auto strView = next();
    int retVal = 0;

    try
    {
        retVal = strView.empty() ? 0 : std::stoi(std::string(strView));
    }
    catch(const std::exception& e)
    {
        throw std::logic_error("Cannot convert '" + std::string(strView) + "' to integer, Item [" + std::to_string(m_currentIndex) + "] in line [" + std::to_string(m_lineNumber) + "]");
    }

    return retVal;
}

template <typename DelimeterType>
unsigned int DelimiterTextParser<DelimeterType>::nextUInt() const
{
    auto strView = next();
    unsigned int retVal = 0;

    try
    {
        retVal = strView.empty() ? 0U : std::stoul(std::string(strView));
    }
    catch(const std::exception& e)
    {
        throw std::logic_error("Cannot convert '" + std::string(strView) + "' to unsigned integer, Item [" + std::to_string(m_currentIndex) + "] in line [" + std::to_string(m_lineNumber) + "]");
    }

    return retVal;
}

template <typename DelimeterType>
long long DelimiterTextParser<DelimeterType>::nextLongLong() const
{
    auto strView = next();
    long long retVal = 0;

    try
    {
        retVal = strView.empty() ? 0U : std::stoll(std::string(strView));
    }
    catch(const std::exception& e)
    {
        throw std::logic_error("Cannot convert '" + std::string(strView) + "' to long long, Item [" + std::to_string(m_currentIndex) + "] in line [" + std::to_string(m_lineNumber) + "]");
    }

    return retVal;
}

template <typename DelimeterType>
unsigned long long DelimiterTextParser<DelimeterType>::nextULongLong() const
{
    auto strView = next();
    unsigned long long retVal = 0;

    try
    {
        retVal = strView.empty() ? 0U : std::stoull(std::string(strView));
    }
    catch(const std::exception& e)
    {
        throw std::logic_error("Cannot convert '" + std::string(strView) + "' to unsigned long long, Item [" + std::to_string(m_currentIndex) + "] in line [" + std::to_string(m_lineNumber) + "]");
    }

    return retVal;
}

template <typename DelimeterType>
bool DelimiterTextParser<DelimeterType>::nextBool() const
{
    auto strView = next();
    bool retVal = false;

    try
    {
        retVal = strView.empty() ? false : std::stoi(std::string(strView)) == 1;
    }
    catch(const std::exception& e)
    {
        throw std::logic_error("Cannot convert '" + std::string(strView) + "' to boolean, Item [" + std::to_string(m_currentIndex) + "] in line [" + std::to_string(m_lineNumber ) + "]");
    }

    return retVal;
}

template <typename DelimeterType>
double DelimiterTextParser<DelimeterType>::nextDouble() const
{
    auto strView = next();
    double retVal = 0.0;

    try
    {
        retVal = strView.empty() ? 0 : std::stod(std::string(strView));
    }
    catch(const std::exception& e)
    {
        throw std::logic_error("Cannot convert '" + std::string(strView) + "' to double, Item [" + std::to_string(m_currentIndex) + "] in line [" + std::to_string(m_lineNumber ) + "]");
    }

    return retVal;
}

template <typename DelimeterType>
float DelimiterTextParser<DelimeterType>::nextFloat() const
{
    auto strView = next();    
    float retVal = 0.0F;

    try
    {
        retVal = strView.empty() ? 0 : std::stof(std::string(strView));
    }
    catch(const std::exception& e)
    {
        throw std::logic_error("Cannot convert '" + std::string(strView) + "' to float, Item [" + std::to_string(m_currentIndex + 1 ) + "] in line [" + std::to_string(m_lineNumber ) + "]");
    }
    
    return retVal;
}

template <typename DelimeterType>
std::time_t DelimiterTextParser<DelimeterType>::nextDateTime(const std::string& format) const
{
    auto strView = next();
    std::time_t retVal = Utils::getDateTime(std::string(strView), format);

    if (retVal == -1)
    {
        throw std::logic_error("Cannot convert '" + std::string(strView) + "' to time format " + format + ", Item [" + std::to_string(m_currentIndex) + "] in line [" + std::to_string(m_lineNumber ) + "]");
    }
    
    return retVal;
}

template <typename DelimeterType>
inline void DelimiterTextParser<DelimeterType>::logOutOfRange() const
{
    throw std::logic_error("Item [" + std::to_string(m_currentIndex) + "] does not exist in line [" + std::to_string(m_lineNumber ) + "]");
}

template class lu::utils::DelimiterTextParser<char>;
template class lu::utils::DelimiterTextParser<std::string>;