#include <bpms/Utils.h>
#include <string.h>
#include <util/CLog.hh>
#include <util/CLogFactory.hh>
#include <iostream>

using namespace bpms;

util::CLog* Utils::m_logger;

std::time_t Utils::getDateTime(const std::string &dateTimeStr, const std::string& format)
{
    std::tm tm{};
    memset(&tm, 0, sizeof(tm));
    char* retVal = strptime(dateTimeStr.c_str(), format.c_str(), &tm);

    if (retVal == nullptr)
    {
        return -1;
    }

    return mktime(&tm);
}

std::string Utils::getDateTimeStr(std::time_t time, const std::string& fromat)
{
    std::tm *tm = std::localtime(&time);
    char buffer[64];
    std::strftime(buffer, 64, fromat.c_str(), tm);
    return std::string(buffer);
}

bool Utils::openLogFile(const std::string& logFileName)
{
    std::cout << "[INFO] Log file name " << logFileName << std::endl;
    //util::CLogFactory::cleanUp();
    m_logger = util::CLogFactory::getLogger(logFileName);
    if(m_logger != nullptr)
    {
      m_logger->info() << "Logfiles opened" << std::endl;
      return true;
    }

    return false;
}

util::CLog &Utils::getLogger()
{
    if (m_logger == nullptr)
    {
        m_logger = util::CLogFactory::getLogger(util::DEBUG_MODE);
    }

    return *m_logger;
}