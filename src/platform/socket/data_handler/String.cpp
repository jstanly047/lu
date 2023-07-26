#include <cstddef>
#include <platform/socket/data_handler/String.h>

using namespace lu::platform::socket::data_handler;

String::String() : 
    m_bufferSize(1000),
    m_buffer(new uint8_t[m_bufferSize])
{
}

unsigned int String::readHeader(std::size_t offset)
{
    Header* header = reinterpret_cast<Header*>(m_buffer.get() + offset);
    return header->getSize();
}

void* String::readMessage(std::size_t offset, std::size_t size)
{
    return (void*) new std::string(m_buffer.get() + offset, m_buffer.get() + offset + size);
}