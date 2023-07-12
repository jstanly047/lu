#pragma once

#include <socket/BaseSocket.h>
#include <socket/defs.h>
#include <string>
class Person;
struct sockaddr;

namespace lu::socket
{
    template<typename DataHandler>
    class DataSocket : public BaseSocket
    {
        constexpr static unsigned int RECEIVE_BUFF_SIZE = TCP_MTU;
    public:
        DataSocket(int socketID);
        DataSocket(int socketID, const struct sockaddr& );
        bool Receive();
        int sendMsg(void* buffer, int size);
        int sendFile(int fileDescriptor, int size);

    private:
        inline void readMessages();
        inline void updateForDataRead(uint32_t size);

        DataHandler m_dataHandler;
        unsigned int m_headerSize{};
        unsigned int m_receiveBufferShiftSize{};
        unsigned int m_numberOfBytesInBuffer{};
        unsigned int m_readOffset{};
        unsigned int m_numberOfBytesLeftToRead{};
        unsigned int m_numberOfBytesLeftToRecv{};
    };
}