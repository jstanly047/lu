

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
        virtual bool onInit() = 0;
        virtual void onStart() = 0;
        virtual void onExit() = 0;
        virtual void onNewConnection(lu::platform::socket::DataSocket<IClientThreadCallback, lu::platform::socket::data_handler::String>* dataSocket) = 0;
        virtual void onTimer(const lu::platform::FDTimer<IClientThreadCallback>&) = 0;
        virtual void onClientClose(lu::platform::socket::DataSocket<IClientThreadCallback, lu::platform::socket::data_handler::String>&) = 0;
        virtual void onData(lu::platform::socket::DataSocket<IClientThreadCallback, lu::platform::socket::data_handler::String>&, void* ) = 0;
        void setThread(LuThread& thread) { m_thread = &thread; }
        LuThread& getThread() { return *m_thread; }

    private:
        LuThread* m_thread;
    };
}