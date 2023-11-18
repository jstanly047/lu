#include <utils/Utils.h>
#include <array>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>


using namespace lu::utils;
//constexpr size_t MAX_TIME_STRING_SIZE = 64;

std::time_t Utils::getUTCDateTime(const std::string &dateTimeStr, const std::string& format)
{
    std::tm time{};
    memset(&time, 0, sizeof(time));
    char* retVal = ::strptime(dateTimeStr.c_str(), format.c_str(), &time);

    if (retVal == nullptr)
    {
        return -1;
    }

    return ::timegm(&time);
}

std::time_t Utils::getDateTime(const std::string &dateTimeStr, const std::string& format)
{
    std::tm time{};
    memset(&time, 0, sizeof(time));
    char* retVal = ::strptime(dateTimeStr.c_str(), format.c_str(), &time);

    if (retVal == nullptr)
    {
        return -1;
    }

    return std::mktime(&time);
}
/*
std::string Utils::getDateTimeStr(std::time_t time, const std::string& format)
{
    std::tm *tm = std::localtime(&time); // Not thread safe
    std::string buffer;
    buffer.reserve(MAX_TIME_STRING_SIZE);
    std::strftime(buffer.data(), MAX_TIME_STRING_SIZE, format.c_str(), tm);
    return buffer;
}
*/
bool Utils::readDataSocket(int socketId, uint8_t *buf, size_t size, ssize_t &readCount)
{
    readCount = ::recv(socketId, buf, size, MSG_DONTWAIT);

    if (readCount < 0)
    {
        readCount = 0;
        return errno == EAGAIN || errno == EWOULDBLOCK;
    }

    return readCount != 0;
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

        readCount = 0;
        return errno == EAGAIN;
    }

    return readCount != 0;
}

bool Utils::readDataFile(int socketId, uint8_t *buf, size_t size, ssize_t &readCount)
{
    // TODO we can try replace this by ::readv (scatter/gather IO)
    readCount = ::read(socketId, buf, size);

    if (readCount < 0)
    {
        readCount = 0;
        return errno == EAGAIN || errno == EWOULDBLOCK;
    }

    return readCount != 0;
}
