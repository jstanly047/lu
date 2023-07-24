#pragma once

#include <cstddef>
#include <platform/socket/DataSocket.h>

namespace lu::platform::socket
{
    template<lu::common::NonPtrClassOrStruct DataHandler>
    class IDataSocketCallback
    {
    public:

        virtual void onClientClose(const lu::platform::socket::DataSocket<IDataSocketCallback, DataHandler>& baseSocket) {}
        virtual void onMessage(const lu::platform::socket::DataSocket<IDataSocketCallback, DataHandler>& baseSocket, void* data) {}
    };
}
