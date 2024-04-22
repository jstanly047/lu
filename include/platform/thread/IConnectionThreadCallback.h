#pragma once
#include <platform/FDTimer.h>
#include <platform/socket/DataSocket.h>
#include <platform/socket/data_handler/String.h>
#include <platform/socket/ConnectSocket.h>
#include <platform/thread/channel/InputChannel.h>

namespace lu::platform::thread
{
    class IConnectionThreadCallback
    {
    public:
        using DataSocketType=lu::platform::socket::DataSocket<IConnectionThreadCallback, lu::platform::socket::data_handler::String>;
        using SSLDataSocketType=lu::platform::socket::DataSocket<IConnectionThreadCallback, lu::platform::socket::data_handler::String, lu::platform::socket::SSLSocket>;
        virtual bool onInit() = 0;
        virtual void onStart() = 0;
        //virtual void onStartComplete() = 0;
        virtual void onExit() = 0;
        virtual void onTimer(const lu::platform::FDTimer<IConnectionThreadCallback>&) = 0;
        virtual void onConnection(DataSocketType& ) = 0;
        virtual void onConnection(SSLDataSocketType& ) = 0;
        virtual void onClientClose(DataSocketType&) = 0;
        virtual void onClientClose(SSLDataSocketType&) = 0;
        virtual void onData(DataSocketType&, void* ) = 0;
        virtual void onData(SSLDataSocketType&, void* ) = 0;
        virtual void onAppMsg(void*, lu::platform::thread::channel::ChannelID ) = 0;
    };
}