#pragma once

#include <cstddef>
#include <platform/socket/DataSocket.h>
#include <platform/socket/IDataHandler.h>

namespace lu::platform::socket
{

    class IConnectionHandler
    {
    public:
        virtual void onNewConnection(BaseSocket* dataSocket) =  0;
    };
}