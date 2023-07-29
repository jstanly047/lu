

#pragma once
#include <platform/FDTimer.h>

namespace lu::platform::thread
{
    class IEventThreadCallback
    {
    public:
        virtual bool onInit() = 0;
        virtual void onStart() = 0;
        virtual void onStartComplete() = 0;
        virtual void onExit() = 0;
        virtual void onTimer(const lu::platform::FDTimer<IEventThreadCallback>&) = 0;
    };
}