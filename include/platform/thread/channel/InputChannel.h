#pragma once

#include <platform/thread/channel/TransferQueue.h>
#include <vector>
#include <string>
#include <map>

namespace lu::platform::thread::channel
{
    class InputChannel
    {
    public:
        InputChannel();
        TransferQueue& getTransferQueue() { return *m_transferQueue; }
        void sendStop();
        ~InputChannel();
        void push(ChannelData);
        ChannelData pop();

    private:
        TransferQueue* m_transferQueue;
    };
}