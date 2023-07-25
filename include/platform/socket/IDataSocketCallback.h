#pragma once

#include <cstddef>
#include <platform/socket/DataSocket.h>
#include <platform/socket/IDataHandler.h>

namespace lu::platform::socket
{
    class IDataSocketCallback
    {
    public:

        virtual void onClientClose([[maybe_unused]] const DataSocket<IDataSocketCallback, IDataHandler>& baseSocket) {}
        virtual void onMessage([[maybe_unused]] const DataSocket<IDataSocketCallback, IDataHandler>& baseSocket, [[maybe_unused]] void* data) {}
    };
}