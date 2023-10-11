#include <cstddef>
#include <common/defs.h>
#include <platform/socket/data_handler/StringV.h>

using namespace lu::platform::socket::data_handler;

StringV::StringV() : 
    m_buffer1(new uint8_t[BUFFER_SIZE]),
    m_buffer2(new uint8_t[BUFFER_SIZE])
{
    m_readIOVec[0].iov_base = m_buffer1.get();
    m_readIOVec[0].iov_len = BUFFER_SIZE;
    m_readIOVec[1].iov_base = m_buffer2.get();
    m_readIOVec[1].iov_len = BUFFER_SIZE;
}

ssize_t StringV::readHeader(ssize_t offset)
{
    if ((offset < BUFFER_SIZE) && (offset + (ssize_t) sizeof(Header) >=  BUFFER_SIZE))
    {
        auto bytesInFirstBuffer = BUFFER_SIZE - offset;
        Header header;
        std::memcpy(&header, m_buffer1.get() + offset, bytesInFirstBuffer);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::memcpy(reinterpret_cast<uint8_t*>(&header) + bytesInFirstBuffer, m_buffer2.get(), (ssize_t) sizeof(Header) - bytesInFirstBuffer);
        return header.getSize();
    }
    if (offset < BUFFER_SIZE)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
        auto* header = reinterpret_cast<Header*>(m_buffer1.get() + offset);
        return header->getSize();
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto* header = reinterpret_cast<Header*>(m_buffer2.get() + offset - BUFFER_SIZE);
    return header->getSize();
}

void* StringV::readMessage(ssize_t offset, ssize_t size)
{
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    auto *newMessage = new Message();

    if ((offset < BUFFER_SIZE) && (offset + size >= BUFFER_SIZE))
    {        
        auto bytesInFirstBuffer = BUFFER_SIZE - offset;
        std::memcpy(newMessage, m_buffer1.get() + offset, bytesInFirstBuffer);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::memcpy(reinterpret_cast<uint8_t*>(newMessage) + bytesInFirstBuffer, m_buffer2.get(), size - bytesInFirstBuffer);
    }
    else if (offset < BUFFER_SIZE)
    {
        std::memcpy(newMessage, m_buffer1.get() + offset, size);
    }
    else
    {
        std::memcpy(newMessage, m_buffer2.get() + offset - BUFFER_SIZE, size);
    }

    return newMessage;
}

int StringV::getNumberOfBuffers(ssize_t writeableOffset)
{
    if (LIKELY(writeableOffset < BUFFER_SIZE))
    {
        m_readIOVec[0].iov_base = m_buffer1.get() + writeableOffset;
        m_readIOVec[0].iov_len = BUFFER_SIZE - writeableOffset;
        m_readIOVec[1].iov_base = m_buffer2.get();
        m_readIOVec[1].iov_len = BUFFER_SIZE;
         return 2;
    }
    
    auto offsetForSecondBuffer =  writeableOffset - BUFFER_SIZE;
    m_readIOVec[0].iov_base = m_buffer2.get() + offsetForSecondBuffer;
    m_readIOVec[0].iov_len = BUFFER_SIZE - offsetForSecondBuffer;
    return 1;
}