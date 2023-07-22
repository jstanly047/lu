#include <utils/Utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>

using namespace lu::utils;

std::time_t Utils::getDateTime(const std::string &dateTimeStr, const std::string& format)
{
    std::tm tm{};
    memset(&tm, 0, sizeof(tm));
    char* retVal = strptime(dateTimeStr.c_str(), format.c_str(), &tm);

    if (retVal == nullptr)
    {
        return -1;
    }

    return std::mktime(&tm);
}

std::string Utils::getDateTimeStr(std::time_t time, const std::string& fromat)
{
    std::tm *tm = std::localtime(&time);
    char buffer[64];
    std::strftime(buffer, 64, fromat.c_str(), tm);
    return std::string(buffer);
}

