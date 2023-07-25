#include <cstddef>
#include <platform/socket/data_handler/String.h>

using namespace lu::platform::socket::data_handler;

template<lu::common::NonPtrClassOrStruct DataSocketCallback>
String<DataSocketCallback>::String(DataSocketCallback& dataSocketCallback, std::size_t bufferSize) : 
    m_bufferSize(bufferSize),
    m_buffer(new uint8_t[bufferSize]),
    m_dataSocketCallback(dataSocketCallback)
{
}

template<lu::common::NonPtrClassOrStruct DataSocketCallback>
unsigned int String<DataSocketCallback>::readHeader(std::size_t offset)
{
    Header* header = reinterpret_cast<Header*>(m_buffer.get() + offset);
    return header->getSize();
}

template<lu::common::NonPtrClassOrStruct DataSocketCallback>
void String<DataSocketCallback>::readMessage(std::size_t offset, std::size_t size)
{
    m_dataSocketCallback.onMessage(new std::string(m_buffer.get() + offset, m_buffer.get() + offset + size));
}

