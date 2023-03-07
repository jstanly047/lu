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

void Utils::DieWithUserMessage(const char *msg, const char *detail) {
    fputs(msg, stderr);
    fputs(": ", stderr);
    fputs(detail, stderr);
    fputc('\n', stderr);
    exit(1);
}

void Utils::DieWithSystemMessage(const char *msg) {
    perror(msg);
    exit(1);
}


void Utils::PrintSocketAddress(const struct sockaddr &address, FILE *stream) {
    void *numericAddress;
    char addrBuffer[INET6_ADDRSTRLEN];
    
    in_port_t port;
    switch (address.sa_family)
    {
        case AF_INET:
        numericAddress = &((struct sockaddr_in &) address).sin_addr;
        port = ntohs(((struct sockaddr_in &) address).sin_port);
        break;
        case AF_INET6:
        numericAddress = &((struct sockaddr_in6 &) address).sin6_addr;
        port = ntohs(((struct sockaddr_in6 &) address).sin6_port);
        break;
        default:
        fputs("[unknown type]", stream);
        return;
    }

    if (inet_ntop(address.sa_family, numericAddress, addrBuffer, sizeof(addrBuffer)) == NULL)
    {
        fputs("[invalid address]", stream);
    }
    else
    {
        fprintf(stream, "%s", addrBuffer);
        if (port != 0)
        {
            fprintf(stream, "-%u", port);
        }
    }
}

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

