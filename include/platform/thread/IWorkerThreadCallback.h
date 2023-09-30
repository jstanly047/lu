

#pragma once
#include <platform/thread/channel/TransferQueue.h>

namespace lu::platform::thread
{
    class IWorkerThreadCallback
    {
    public:
        virtual void onMsg(channel::ChannelData channelData) = 0;
        virtual bool onInit() = 0;
        virtual void onStart() = 0;
        virtual void onExit() = 0;
    };
}