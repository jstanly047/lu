#pragma once
#include <platform/socket/DataSocket.h>
#include <common/TemplateConstraints.h>

namespace lu::platform::socket::data_handler
{
    class String
    {
    public:
        class Header
        {
        public:
            Header(unsigned int size) : m_size(size){}
            unsigned int getSize() const { return m_size; }

        private:
            unsigned int m_size{};
        };

        String(const String&)               = delete;
        String& operator=(const String&)    = delete;
        String(String&& other)              = delete;
        String& operator=(String&& other)   = delete;
        
        String();
        ~String() {}

        
        uint8_t* getReceiveBufferToFill() { return m_buffer.get(); }
        std::size_t getReceiveBufferSize() { return m_bufferSize; }
        std::size_t getHeaderSize() { return sizeof(Header); };
        unsigned int readHeader(std::size_t offset);
        void* readMessage(std::size_t offset, std::size_t size);

    private:
        std::size_t m_bufferSize = 1000;
        std::unique_ptr<uint8_t[]> m_buffer;
    };
}