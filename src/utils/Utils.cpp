#include <utils/Utils.h>
#include <array>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <unistd.h>


using namespace lu::utils;
constexpr size_t MAX_TIME_STRING_SIZE = 64;

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

std::string Utils::getDateTimeStr(std::time_t time, const std::string& format)
{
    std::tm *tm = std::localtime(&time);
    std::string buffer;
    buffer.reserve(MAX_TIME_STRING_SIZE);
    std::strftime(buffer.data(), MAX_TIME_STRING_SIZE, format.c_str(), tm);
    return buffer;
}

bool Utils::readDataSocket(int socketId, uint8_t *buf, size_t size, ssize_t &readCount)
{
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

bool Utils::readDataSocket(int socketId,struct ::iovec* dataBufferVec, int numOfVBuffers, ssize_t &readCount)
{
    //constexpr off_t USE_CURRENT_OFFSET_AND_UPDATE = -1;
    readCount = ::readv(socketId, dataBufferVec, numOfVBuffers);

    if (readCount < 0)
    {
        if (errno == EOPNOTSUPP)
        {
            std::abort();
        }

        if (errno == EAGAIN)
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

bool Utils::readDataFile(int socketId, uint8_t *buf, size_t size, ssize_t &readCount)
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
