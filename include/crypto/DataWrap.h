#pragma once
#include <memory>

namespace lu::crypto
{
    class DataWrap
    {
    public:
        DataWrap(DataWrap &other) = delete;
        DataWrap &operator=(DataWrap &other) = delete;
        
        DataWrap(std::size_t length);
        DataWrap(DataWrap &&other) noexcept;
        DataWrap &operator=(DataWrap &&other) noexcept;

        const unsigned char* getData() const { return m_data.get(); }
        unsigned char* getData() { return m_data.get(); }
        std::size_t getLength() const { return m_length; }

    private:
        
         
        std::unique_ptr<unsigned char[]> m_data;
        std::size_t m_length;
    };
}