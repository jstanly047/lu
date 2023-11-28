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
        m_data = std::move(other.m_data);
        m_length = other.m_length;
    }

    return *this;
}
