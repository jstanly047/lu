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

bool Utils::readDataSocket(int socketId, uint8_t *buf, size_t size, int32_t &readCount)
{
    // TODO we can try replace this by ::readv (scatter/gather IO)
    readCount = ::recv(socketId, buf, size, MSG_DONTWAIT);

    if (readCount < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            readCount = 0;
            return true;
        }
        else
        {
            readCount = 0;
            return false;
        }
    }
    else if (readCount == 0)
    {
        return false;
    }

    return true;
}

bool Utils::readDataFile(int socketId, uint8_t *buf, size_t size, int32_t &readCount)
{
    // TODO we can try replace this by ::readv (scatter/gather IO)
    readCount = ::read(socketId, buf, size);

    if (readCount < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            readCount = 0;
            return true;
        }
        else
        {
            readCount = 0;
            return false;
        }
    }
    else if (readCount == 0)
    {
        return false;
    }

    return true;
}
