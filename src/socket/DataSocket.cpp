#include<socket/DataSocket.h>
#include<string>
#include<sys/socket.h>
#include <arpa/inet.h>
#include<stdio.h>
#include <utils/Utils.h>
#include <cstring>
#include <sys/sendfile.h>
#include <assert.h>

using namespace lu::socket;

namespace
{
    inline bool receiveMsg(int socketId, uint8_t* buf, size_t size, int32_t& readCount)
    {
        readCount = recv(socketId, buf, size, MSG_DONTWAIT);
    
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
}

template<typename DataHandler>
DataSocket<DataHandler>::DataSocket(int socketID) : BaseSocket(socketID)
{
    m_headerSize = m_dataHandler.getHeaderSize();
    m_receiveBufferShiftSize = m_dataHandler.getReceiveBufferSize() / 4;
    m_numberOfBytesLeftToRecv = m_dataHandler.getReceiveBufferSize();
}

template<typename DataHandler>
DataSocket<DataHandler>::DataSocket(int socketID, const struct sockaddr& address): BaseSocket(socketID)
{
    getIPAndPort(address);
}

template<typename DataHandler>
void DataSocket<DataHandler>::updateForDataRead(uint32_t size)
{
    m_readOffset += size;
    m_numberOfBytesLeftToRead -= size;
}

template<typename DataHandler>
bool DataSocket<DataHandler>::Receive()
{
    int numberOfBytesRead = 0;

    if (receiveMsg(m_socketId, m_dataHandler.getReceiveBufferToFill() + m_numberOfBytesLeftToRead, m_numberOfBytesLeftToRecv, numberOfBytesRead) == false)
    {
        return false;
    }

    m_numberOfBytesLeftToRead += (uint32_t) numberOfBytesRead;
    readMessages();

    if (m_numberOfBytesLeftToRead == 0)
    {
        m_readOffset = 0;
        m_numberOfBytesLeftToRecv = m_dataHandler.getReceiveBufferSize();
    }
    else if (m_numberOfBytesLeftToRead <= m_receiveBufferShiftSize)
    {
        std::memcpy(m_dataHandler.getReceiveBufferToFill(), m_dataHandler.getReceiveBufferToFill() + m_readOffset, m_numberOfBytesLeftToRead);
        m_readOffset = 0;
        m_numberOfBytesLeftToRecv =  m_dataHandler.getReceiveBufferSize() - m_numberOfBytesLeftToRead;
        assert(m_numberOfBytesLeftToRecv > 0u);
    }

    return true;
}

template<typename DataHandler>
void DataSocket<DataHandler>::readMessages()
{
    unsigned int expectedMsgSize = 0;

    for(;;)
    {
        if (m_numberOfBytesLeftToRead >= m_headerSize)
        {
            // TODO the reader function must handle the alignment and endianess 
            auto header = m_dataHandler.readHeader(m_readOffset);
            expectedMsgSize = header.getMessageSize();
            updateForDataRead(sizeof(uint32_t));
        }
        else
        {
            return;
        }

        if (m_numberOfBytesLeftToRead >= expectedMsgSize)
        {
            m_dataHandler.readMessage(m_readOffset, expectedMsgSize);
            updateForDataRead(expectedMsgSize);
            continue;
        }

        return;
    }
}

template<typename DataHandler>
int DataSocket<DataHandler>::sendMsg(void* buffer, int size)
{
    if (m_socketId == NULL_SOCKET)
    {
        return false;
    }

    int totalSent = 0;

    while (totalSent < size)
    {
        ssize_t numBytesSend = send(m_socketId, buffer, size, MSG_DONTWAIT);

        if (numBytesSend < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }
            
            return totalSent;
        }
        else if (numBytesSend != (ssize_t) size)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR 
                    || errno == ENOBUFS)
            {
                totalSent += numBytesSend;
                size -= numBytesSend;
                continue;
            }

            return totalSent;
        }

        totalSent += numBytesSend;
    }

    return totalSent;
}

template<typename DataHandler>
int DataSocket<DataHandler>::sendFile(int fileDescriptor, int size)
{
    off_t offset = 0;
    ssize_t sendBytes = sendfile(m_socketId, fileDescriptor, &offset, size);

    if (sendBytes == -1) 
    {
        return false;
    },

    int totalSent = 0;

    while (totalSent < size)
    {
        ssize_t numBytesSend = sendfile(m_socketId, fileDescriptor, &offset, size);

        if (numBytesSend < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }
            
            return totalSent;
        }
        else if (numBytesSend != (ssize_t) size)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR 
                    || errno == ENOBUFS)
            {
                totalSent += numBytesSend;
                size -= numBytesSend;
                continue;
            }

            return totalSent;
        }

        totalSent += numBytesSend;
    }

    return sendBytes;
}
