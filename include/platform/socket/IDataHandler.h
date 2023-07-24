#pragma once

#include <cstddef>
#include <platform/socket/IDataSocketCallback.h>

namespace lu::platform::socket
{
    class IDataHandler
    {
    public:
        IDataHandler(IDataHandler&& other) noexcept: 
            m_iDataSocketCallback(other.m_iDataSocketCallback) {}
            
        IDataHandler& operator=(IDataHandler&& other)
        {
            std::swap(m_iDataSocketCallback, other.m_iDataSocketCallback);
            return *this;
        }

        IDataHandler(IDataSocketCallback<IDataHandler>& iDataSocketCallback) :  m_iDataSocketCallback(iDataSocketCallback) {}
        virtual uint8_t* getReceiveBufferToFill() { return nullptr; }
        virtual std::size_t getReceiveBufferSize() { return 0; }
        virtual std::size_t getHeaderSize() { return 0; }
        virtual std::size_t readHeader(std::size_t offset) {return 0; }
        virtual void readMessage(std::size_t offset, std::size_t size) {}

    private:
        IDataSocketCallback<IDataHandler>& m_iDataSocketCallback;
        
    };
}