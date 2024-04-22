

#pragma once
#include <platform/FDTimer.h>
#include <platform/socket/DataSocket.h>
#include <platform/socket/data_handler/String.h>
#include <platform/thread/LuThread.h>

namespace lu::platform::thread
{
    class IClientThreadCallback
    {
    public:
        using DataSocketType=lu::platform::socket::DataSocket<IClientThreadCallback, lu::platform::socket::data_handler::String>;
        using SSLDataSocketType=lu::platform::socket::DataSocket<IClientThreadCallback, lu::platform::socket::data_handler::String, lu::platform::socket::SSLSocket>;

        virtual bool onInit() = 0;
        virtual void onStart() = 0;
        virtual void onExit() = 0;
        virtual void onNewConnection(DataSocketType& dataSocket) = 0;
        virtual void onNewConnection(SSLDataSocketType& dataSocket) = 0;
        virtual void onAppMsg(void* msg, lu::platform::thread::channel::ChannelID) = 0;
        virtual void onTimer(const lu::platform::FDTimer<IClientThreadCallback>&) = 0;
        virtual void onClientClose(DataSocketType&) = 0;
        virtual void onClientClose(SSLDataSocketType&) = 0;
        virtual void onData(DataSocketType&, void* ) = 0;
        virtual void onData(SSLDataSocketType&, void* ) = 0;
        LuThread& getThread() { return *m_thread; }

    private:
        LuThread* m_thread;
    };
}