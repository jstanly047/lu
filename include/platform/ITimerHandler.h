#pragma once

#include <cstddef>
#include <platform/FDTimer.h>

namespace lu::platform
{
    class ITimerHandler
    {
    public:
        virtual void onTimer(const lu::platform::FDTimer<ITimerHandler>& ) =  0;
    };
}