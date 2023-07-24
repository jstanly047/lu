#pragma once

namespace lu::platform
{
    class IFDEventHandler;

    class IEventChannelHandler
    {
        void onChannelEvent(int event, void* ptr);
    };
}