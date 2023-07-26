#pragma once

#include <cstddef>
#include <platform/socket/DataSocket.h>
#include <platform/socket/IDataHandler.h>
#include <common/TemplateConstraints.h>
#include <platform/socket/data_handler/String.h>

namespace lu::platform::socket
{
    template<lu::common::NonPtrClassOrStruct DataHandler>
    class IDataSocketCallback
    {
    public:

        virtual void onClientClose([[maybe_unused]] DataSocket<IDataSocketCallback, DataHandler>& dataSocket) {}
        virtual void onData([[maybe_unused]] DataSocket<IDataSocketCallback, DataHandler>& dataSocket, [[maybe_unused]] void* data) {}
    };
}

template class lu::platform::socket::IDataSocketCallback<lu::platform::socket::IDataHandler>;
template class lu::platform::socket::IDataSocketCallback<lu::platform::socket::data_handler::String>;
