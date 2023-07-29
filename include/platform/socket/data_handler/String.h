#pragma once
#include <platform/socket/DataSocket.h>
#include <common/TemplateConstraints.h>
#include <cstring>

namespace lu::platform::socket::data_handler
{
    class String
    {
    public:
        class Header
        {
        public:
            Header(){}
            Header(unsigned int size) : m_size(size){}
            unsigned int getSize() const { return m_size; }
        private:
            unsigned int m_size{};
        };

        class Message : public Header
        {
        public:
            Message(){}
            Message(const std::string& data) : Header(sizeof(Message))
            {
                std::strncpy(m_data, data.data(), data.length());
                m_data[ data.length()] = '\0';
            }

            std::string getString() const { return std::string(m_data); }

        private:
            char m_data[64];
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