#pragma once

#include <cstdint>
#include <cassert>
#include <common/TemplateConstraints.h>

namespace lu::platform::socket
{
    template<lu::common::NonPtrClassOrStruct DataSocketCallback>
    class IDataHandler
    {
    public:
        IDataHandler(DataSocketCallback& dataSocketCallback);
        IDataHandler(IDataHandler&& other) noexcept;
        IDataHandler& operator=(IDataHandler&& other);

        virtual uint8_t* getReceiveBufferToFill() { return nullptr; }
        virtual std::size_t getReceiveBufferSize() { return 0; }
        virtual std::size_t getHeaderSize() { return 0; }
        virtual std::size_t readHeader(std::size_t offset) {return offset; }
        virtual void readMessage([[maybe_unused]] std::size_t offset, [[maybe_unused]] std::size_t size) {   }

    private:
        DataSocketCallback& m_dataSocketCallback;
    };
}