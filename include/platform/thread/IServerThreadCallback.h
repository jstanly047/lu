#pragma once
#include <platform/socket/DataSocket.h>
#include <platform/socket/data_handler/String.h>
#include <platform/FDTimer.h>

namespace lu::platform::thread
{
    class IServerThreadCallback
    {
    public:
        virtual bool onInit() = 0;
        virtual void onStart() = 0;
        //virtual void onStartComplete() = 0;
        virtual void onExit() = 0;
        virtual void onNewConnection(lu::platform::socket::BaseSocket* baseSocket) = 0;
        virtual void onTimer(const lu::platform::FDTimer<IServerThreadCallback>&) = 0;
    };
}