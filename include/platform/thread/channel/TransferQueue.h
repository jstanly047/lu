
#pragma once

#include <queue/Queue.h>

namespace lu::platform::thread::channel
{
    using ChannelID = unsigned int;
    struct ChannelData
    {
        ChannelID channelID;
        void * data;
    };

    using TransferQueue = lu::queue::RetryDecorator<lu::queue::AtomicQueue2<ChannelData, 10000u> >;
}