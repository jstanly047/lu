#include<platform/socket/DataSocket.h>
#include<platform/socket/IDataHandler.h>
#include<platform/socket/IDataSocketCallback.h>
#include <platform/defs.h>
#include <utils/Utils.h>

#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#include <assert.h>
#include<stdio.h>
#include <cstring>
#include<string>



using namespace lu::platform::socket;

namespace
{
    inline bool readData(int socketId, uint8_t* buf, size_t size, int32_t& readCount)
    {
        //TODO we can try replace this by ::readv (scatter/gather IO)
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
}

template<lu::common::NonPtrClassOrStruct DataSocketCallback, lu::common::NonPtrClassOrStruct DataHandler>
DataSocket<DataSocketCallback, DataHandler>::DataSocket(DataSocketCallback& dataSocketCallback, BaseSocket&& baseSocket) :
    m_baseSocket(std::move(baseSocket)),
    m_dataSocketCallback(dataSocketCallback),
    m_dataHandler(dataSocketCallback),
    m_headerSize(m_dataHandler.getHeaderSize()),
    m_receiveBufferShiftSize(m_dataHandler.getReceiveBufferSize() / 4),
    m_numberOfBytesLeftToRecv(m_dataHandler.getReceiveBufferSize())

{
}

template<lu::common::NonPtrClassOrStruct DataSocketCallback, lu::common::NonPtrClassOrStruct DataHandler>
DataSocket<DataSocketCallback, DataHandler>::DataSocket(DataSocket<DataSocketCallback, DataHandler>&& other) noexcept : 
    m_baseSocket(std::move(other.m_baseSocket)),
    m_dataSocketCallback(other.m_dataSocketCallback),
    m_dataHandler(std::move(other.m_dataHandler)),
    m_headerSize(std::move(other.m_headerSize)),
    m_receiveBufferShiftSize(std::move(other.m_receiveBufferShiftSize)),
    m_numberOfBytesInBuffer(std::move(other.m_numberOfBytesInBuffer)),
    m_readOffset(std::move(other.m_readOffset)),
    m_numberOfBytesLeftToRead(std::move(other.m_numberOfBytesLeftToRead)),
    m_numberOfBytesLeftToRecv(std::move(other.m_numberOfBytesLeftToRecv))
{
}

template<lu::common::NonPtrClassOrStruct DataSocketCallback, lu::common::NonPtrClassOrStruct DataHandler>
DataSocket<DataSocketCallback, DataHandler>& DataSocket<DataSocketCallback, DataHandler>::operator=(DataSocket<DataSocketCallback, DataHandler>&& other) noexcept
{
    m_baseSocket = std::move(other.m_baseSocket);
    std::swap(m_dataSocketCallback, other.m_dataSocketCallback);
    m_dataHandler = std::move(other.m_dataHandler);
    std::swap(m_headerSize, other.m_headerSize);
    std::swap(m_receiveBufferShiftSize, other.m_receiveBufferShiftSize);
    std::swap(m_numberOfBytesInBuffer, other.m_numberOfBytesInBuffer);
    std::swap(m_readOffset, other.m_readOffset);
    std::swap(m_numberOfBytesLeftToRead, other.m_numberOfBytesLeftToRead);
    std::swap(m_numberOfBytesLeftToRecv, other.m_numberOfBytesLeftToRecv);
    return *this;
}

template<lu::common::NonPtrClassOrStruct DataSocketCallback, lu::common::NonPtrClassOrStruct DataHandler>
void DataSocket<DataSocketCallback, DataHandler>::updateForDataRead(std::size_t size)
{
    m_readOffset += size;
    m_numberOfBytesLeftToRead -= size;
}

template<lu::common::NonPtrClassOrStruct DataSocketCallback, lu::common::NonPtrClassOrStruct DataHandler>
bool DataSocket<DataSocketCallback, DataHandler>::Receive()
{
    int numberOfBytesRead = 0;

    if (readData(m_baseSocket.getFD(), m_dataHandler.getReceiveBufferToFill() + m_numberOfBytesLeftToRead, m_numberOfBytesLeftToRecv, numberOfBytesRead) == false)
    {
        return false;
    }

    m_numberOfBytesLeftToRead += numberOfBytesRead;
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

template<lu::common::NonPtrClassOrStruct DataSocketCallback, lu::common::NonPtrClassOrStruct DataHandler>
void DataSocket<DataSocketCallback, DataHandler>::readMessages()
{
    unsigned int expectedMsgSize = 0;

    for(;;)
    {
        if (m_numberOfBytesLeftToRead >= m_headerSize)
        {
            // TODO the reader function must handle the alignment and endianess
            expectedMsgSize = m_dataHandler.readHeader(m_readOffset);
            updateForDataRead(m_headerSize);
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

template<lu::common::NonPtrClassOrStruct DataSocketCallback, lu::common::NonPtrClassOrStruct DataHandler>
int DataSocket<DataSocketCallback, DataHandler>::sendMsg(void* buffer, ssize_t size)
{
    if (m_baseSocket.getFD() == nullptr)
    {
        return false;
    }

    ssize_t totalSent = 0;

    while (totalSent < size)
    {
        //TODO we can try replace this by ::writev (scatter/gather IO)
        ssize_t numBytesSend = ::send(m_baseSocket.getFD(), buffer, size, MSG_DONTWAIT);

        if (numBytesSend < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }
            
            return totalSent;
        }
        else if (numBytesSend != size)
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

template<lu::common::NonPtrClassOrStruct DataSocketCallback, lu::common::NonPtrClassOrStruct DataHandler>
int DataSocket<DataSocketCallback, DataHandler>::sendFile(int fileDescriptor, int size)
{
    off_t offset = 0;
    ssize_t sendBytes = ::sendfile(m_baseSocket.getFD(), fileDescriptor, &offset, size);

    if (sendBytes == -1) 
    {
        return false;
    }

    int totalSent = 0;

    while (totalSent < size)
    {
        ssize_t numBytesSend = ::sendfile(m_baseSocket.getFD(), fileDescriptor, &offset, size);

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

template<lu::common::NonPtrClassOrStruct DataSocketCallback, lu::common::NonPtrClassOrStruct DataHandler>
void DataSocket<DataSocketCallback, DataHandler>::onEvent(struct ::epoll_event& events)
{
    if ((events.events & EPOLLHUP || events.events & EPOLLERR))
    {
        m_dataSocketCallback.onClientClose(*this);
        return;
    }
    else if (!(events.events & EPOLLIN))
    {
        return;
    }

    Receive();
}

template class lu::platform::socket::DataSocket<lu::platform::socket::IDataSocketCallback<lu::platform::socket::IDataHandler>, lu::platform::socket::IDataHandler>;