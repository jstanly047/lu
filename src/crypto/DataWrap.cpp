#include <crypto/DataWrap.h>
#include <cstring>

using namespace lu::crypto;

DataWrap::DataWrap(std::size_t length) : m_data(new unsigned char[length]),
                                 m_length(length)
{
    std::memset(m_data.get(), 0, m_length);
}

DataWrap::DataWrap(DataWrap &&other) noexcept
    : m_data(std::move(other.m_data)), m_length(other.m_length)
{
}

DataWrap &DataWrap::operator=(DataWrap &&other) noexcept
{
    if (this != &other)
    {
        m_data.swap(other.m_data);
        std::swap(m_length, other.m_length);
    }

    return *this;
}

bool DataWrap::append(const void* data, std::size_t length)
{
    if (m_offset + length > m_length)
    {
        return false;
    }

    std::memcpy(m_data.get() + m_offset, data, length);
    m_offset += length;
    return true;
}

void DataWrap::clear() noexcept
{
    m_offset = 0;
}
