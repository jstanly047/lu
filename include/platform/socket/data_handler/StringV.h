#pragma once
#include <platform/socket/DataSocket.h>
#include <common/TemplateConstraints.h>
#include <cstring>
#include <sys/uio.h>


namespace lu::platform::socket::data_handler
{
    class StringV
    {
    public:
        static constexpr ssize_t BUFFER_SIZE=1000;
        #pragma pack(push, 1) 
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
            char m_data[64]{};
        };
        #pragma pack(pop)

        StringV(const StringV&)               = delete;
        StringV& operator=(const StringV&)    = delete;
        StringV(StringV&& other)              = delete;
        StringV& operator=(StringV&& other)   = delete;
        
        StringV();
        ~StringV() {}

        
        uint8_t* getReceiveBufferToFill() { return m_buffer1.get(); }
        ssize_t getReceiveBufferSize() { return BUFFER_SIZE * 2; }
        ssize_t getSingleBufferSize() { return BUFFER_SIZE; }
        ssize_t getHeaderSize() { return sizeof(Header); }
        struct iovec* getIOVect() { return m_readIOVec; }
        void swap() { m_buffer1.swap(m_buffer2);}
        ssize_t readHeader(ssize_t offset);
        void* readMessage(ssize_t offset, ssize_t size);
        int getNumberOfBuffers(ssize_t writeableOffset);
        

    private:
        std::unique_ptr<uint8_t[]> m_buffer1;
        std::unique_ptr<uint8_t[]> m_buffer2;
        struct iovec m_readIOVec[2]{};
    };
}