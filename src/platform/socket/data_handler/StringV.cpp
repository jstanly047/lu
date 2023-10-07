#include <cstddef>
#include <common/defs.h>
#include <platform/socket/data_handler/StringV.h>

using namespace lu::platform::socket::data_handler;

StringV::StringV() : 
    m_bufferSize(1000),
    m_buffer1(new uint8_t[m_bufferSize]),
    m_buffer2(new uint8_t[m_bufferSize])
{
    m_readIOVec[0].iov_base = m_buffer1.get();
    m_readIOVec[0].iov_len = m_bufferSize;
    m_readIOVec[1].iov_base = m_buffer2.get();
    m_readIOVec[1].iov_len = m_bufferSize;
}

ssize_t StringV::readHeader(ssize_t offset)
{
    if ((offset < m_bufferSize) && (offset + (ssize_t) sizeof(Header) >  m_bufferSize))
    {
        auto bytesInFirstBuffer = m_bufferSize - offset;
        Header header;
        std::memcpy(&header, m_buffer1.get() + offset, bytesInFirstBuffer);
        std::memcpy(&header, m_buffer2.get(), (ssize_t) sizeof(Header) - bytesInFirstBuffer);
        return header.getSize();
    }
    else if (offset < m_bufferSize)
    {
        Header* header = reinterpret_cast<Header*>(m_buffer1.get() + offset);
        return header->getSize();
    }

    Header* header = reinterpret_cast<Header*>(m_buffer2.get() + offset);
    return header->getSize();
}

void* StringV::readMessage(ssize_t offset, ssize_t size)
{
    auto newMessage = new Message();

    if ((offset < m_bufferSize) && (offset + size > m_bufferSize))
    {
        auto bytesInFirstBuffer = m_bufferSize - offset;
        std::memcpy(newMessage, m_buffer1.get() + offset, bytesInFirstBuffer);
        std::memcpy(newMessage, m_buffer2.get(), size - bytesInFirstBuffer);
    }
    else if (offset < m_bufferSize)
    {
        std::memcpy(newMessage, m_buffer1.get() + offset, size);
    }
    else
    {
        std::memcpy(newMessage, m_buffer2.get() + offset, size);
    }

    return newMessage;
}

int StringV::getNumberOfBuffers(ssize_t writeableOffset)
{
    if (LIKELY(writeableOffset < m_bufferSize))
    {
        m_readIOVec[0].iov_base = m_buffer1.get() + writeableOffset;
        m_readIOVec[0].iov_len = m_bufferSize - writeableOffset;
        m_readIOVec[1].iov_base = m_buffer2.get();
        m_readIOVec[1].iov_len = m_bufferSize;
         return 2;
    }
    
    auto offsetForSecondBuffer =  writeableOffset - m_bufferSize;
    m_readIOVec[0].iov_base = m_buffer2.get() + offsetForSecondBuffer;
    m_readIOVec[0].iov_len = m_bufferSize - offsetForSecondBuffer;
    return 1;
}