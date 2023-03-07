#pragma once

#include "BaseSocket.h"
#include <string>
class Person;


namespace lu::socket
{
    class DataSocket : public BaseSocket
    {
        constexpr static int RECEIVE_BUFF_SIZE = 128 * 1024;
    public:
        DataSocket(int socketID);

        bool sendMsg(uint8_t* buffer, uint32_t size);
        bool sendFile(int fileDescriptor, uint32_t size);
        std::pair<uint8_t*, uint32_t> getNextMessage();

    private:
        inline void updateForDataRead(uint32_t size);
        
        uint32_t m_expectedMsgSize = 0;
        uint32_t m_numberOfBytesInBuffer = 0;
        uint32_t m_readOffset = 0;
        uint32_t m_numberOfBytesLeftToRead = 0;
        uint32_t m_numberOfBytesLeftToRecv = RECEIVE_BUFF_SIZE;
        uint8_t m_recvBuffer[RECEIVE_BUFF_SIZE];
    };
}