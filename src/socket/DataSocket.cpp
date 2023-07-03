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

DataSocket::DataSocket(int socketID) : BaseSocket(socketID)
{
}

DataSocket::DataSocket(int socketID, const struct sockaddr& address): BaseSocket(socketID)
{
    getIPAndPort(address);
}

bool DataSocket::sendMsg(uint8_t* buffer, uint32_t size)
{
    if (m_socketId == NULL_SOCKET)
    {
        return false;
    }

    auto sendMessage = [&](const void* buf, size_t size)
    {
        ssize_t numBytesSend = send(m_socketId, buf, size, MSG_DONTWAIT);
    
        if (numBytesSend < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return false;
            }
            
            lu::utils::Utils::DieWithSystemMessage("send() failed");
        }
        else if (numBytesSend != (ssize_t) size)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR 
                    || errno == ENOBUFS)
            {
                return false;
            }

            lu::utils::Utils::DieWithUserMessage("send()", "sent unexpected number of bytes");
        }

        return true;
    };
    
    uint32_t networkByteOrderMsgBytesSize = htonl(size);
    sendMessage(&networkByteOrderMsgBytesSize, sizeof(uint32_t));
    sendMessage(buffer, size);
    return true;
}

bool DataSocket::sendFile(int fileDescriptor, uint32_t size)
{
    off_t offset = 0;
    ssize_t sendBytes = sendfile(m_socketId, fileDescriptor, &offset, size);

    if (sendBytes == -1) 
    {
        return false;
    }

    return true;
}

void DataSocket::updateForDataRead(uint32_t size)
{
    m_readOffset += size;
    m_numberOfBytesLeftToRead -= size;
}

std::pair<uint8_t*, uint32_t>  DataSocket::getNextMessage()
{
    auto readMessageFromRcvBuffer = [&]()->std::pair<uint8_t*, uint32_t>
    {
        if (m_expectedMsgSize == 0)
        {
            if (m_numberOfBytesLeftToRead >= sizeof(uint32_t))
            {
                //std::cout << "Before size read numberOfBytesLeftToRead : " << m_numberOfBytesLeftToRead  << " m_readOffset : " << m_readOffset   << " m_expectedMsgSize : " << m_expectedMsgSize << std::endl;
                m_expectedMsgSize = ntohl(*(reinterpret_cast<uint32_t*>(m_recvBuffer + m_readOffset)));
                updateForDataRead(sizeof(uint32_t));
                //std::cout << "After size read numberOfBytesLeftToRead : " << m_numberOfBytesLeftToRead << " m_readOffset : " << m_readOffset   << " m_expectedMsgSize : " << m_expectedMsgSize << std::endl;
            }
            else
            {
                return {nullptr, 0};
            }
        }

        if (m_numberOfBytesLeftToRead >= m_expectedMsgSize)
        {
            //std::cout << "Before size msg numberOfBytesLeftToRead : " << m_numberOfBytesLeftToRead << " m_readOffset : " << m_readOffset  << " m_expectedMsgSize : " << m_expectedMsgSize << std::endl;
            auto currentOffset = m_readOffset;
            updateForDataRead(m_expectedMsgSize);
            auto msgSize = m_expectedMsgSize;
            m_expectedMsgSize = 0;
            //std::cout << "After size msg numberOfBytesLeftToRead : " << m_numberOfBytesLeftToRead    << " m_readOffset : " << m_readOffset     << " m_expectedMsgSize : " << m_expectedMsgSize << std::endl;
            return {m_recvBuffer + currentOffset, msgSize};
        }
        
        return {nullptr, 0};
    };

    auto messageBuffer = readMessageFromRcvBuffer();

    if (messageBuffer.first != nullptr)
    {
        return messageBuffer;
    }

    if (m_numberOfBytesLeftToRead > 0)
    {
        std::memcpy(m_recvBuffer, m_recvBuffer + m_readOffset, m_numberOfBytesLeftToRead);
        m_readOffset = 0;
        m_numberOfBytesLeftToRecv = RECEIVE_BUFF_SIZE - m_numberOfBytesLeftToRead;
        assert(m_numberOfBytesLeftToRecv > 0);
    }
    else
    {
        assert(m_numberOfBytesLeftToRead == 0);
        m_readOffset = 0;
        m_numberOfBytesLeftToRecv = RECEIVE_BUFF_SIZE;
    }

    //std::cout << "Before recv numberOfBytesLeftToRead : " << m_numberOfBytesLeftToRead << " m_readOffset : " << m_readOffset  << std::endl;

    int32_t numberOfBytesRead = 0;   
    if (receiveMsg(m_socketId, m_recvBuffer + m_numberOfBytesLeftToRead, m_numberOfBytesLeftToRecv, numberOfBytesRead) == false)
    {
        return {nullptr, 0};
    }

    m_numberOfBytesLeftToRead += (uint32_t) numberOfBytesRead;
    //std::cout << "After recv numberOfBytesLeftToRead : " << m_numberOfBytesLeftToRead << std::endl;
    return readMessageFromRcvBuffer();
}
