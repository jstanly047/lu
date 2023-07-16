#pragma once

#include <cstddef>
#include <platform/socket/DataSocket.h>

namespace lu::platform::socket
{
    class IDataHandler
    {
    public:
        virtual uint8_t* getReceiveBufferToFill() = 0;
        virtual std::size_t getReceiveBufferSize() = 0;
        virtual std::size_t getHeaderSize() = 0;
        virtual std::size_t readHeader(std::size_t offset) = 0;
        virtual void readMessage(std::size_t offset, std::size_t size) = 0;
        virtual void onClose(lu::platform::socket::DataSocket<IDataHandler>*) =  0;
    };
}