#include <cstddef>
#include <platform/socket/data_handler/String.h>

using namespace lu::platform::socket::data_handler;

String::String() : 
    m_buffer(new uint8_t[BUFFER_SIZE])
{
}

ssize_t String::readHeader(ssize_t offset)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    Header* header = reinterpret_cast<Header*>(m_buffer.get() + offset);
    return header->getSize();
}

void* String::readMessage(ssize_t offset, ssize_t size)
{
    auto newMessage = new Message();
    std::memcpy(newMessage, (void*) (m_buffer.get() + offset), size);
    return newMessage;
}