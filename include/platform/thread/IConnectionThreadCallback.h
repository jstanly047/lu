#pragma once
#include <platform/FDTimer.h>
#include <platform/socket/DataSocket.h>
#include <platform/socket/data_handler/String.h>
#include <platform/socket/ConnectSocket.h>

namespace lu::platform::thread
{
    class IConnectionThreadCallback
    {
    public:
        virtual bool onInit() = 0;
        virtual void onStart() = 0;
        //virtual void onStartComplete() = 0;
        virtual void onExit() = 0;
        virtual void onTimer(const lu::platform::FDTimer<IConnectionThreadCallback>&) = 0;
        virtual void onConnection(lu::platform::socket::ConnectSocket<IConnectionThreadCallback, lu::platform::socket::data_handler::String>& ) = 0;
        virtual void onClientClose(lu::platform::socket::DataSocket<IConnectionThreadCallback, lu::platform::socket::data_handler::String>&) = 0;
        virtual void onData(lu::platform::socket::DataSocket<IConnectionThreadCallback, lu::platform::socket::data_handler::String>&, void* ) = 0;
    };
}