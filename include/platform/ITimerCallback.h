#pragma once

#include <cstddef>
#include <platform/FDTimer.h>

namespace lu::platform
{
    class ITimerCallback
    {
    public:
        virtual void onTimer(const lu::platform::FDTimer<ITimerCallback>& ) =  0;
    };
}