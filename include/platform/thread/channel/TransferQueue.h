
#pragma once


namespace lu::queue
{
    template<class Derived>
    class QueueBase;

    template<class T, unsigned SIZE, bool MINIMIZE_CONTENTION, bool MAXIMIZE_THROUGHPUT, bool TOTAL_ORDER, bool SPSC >
    class AtomicQueue2;

    template<class Queue>
    struct RetryDecorator;
}

namespace lu::platform::thread::channel
{
    using ChannelID = unsigned int;
    struct ChannelData
    {
        ChannelID channelID;
        void * data;
    };

    using TransferQueue = lu::queue::RetryDecorator<lu::queue::AtomicQueue2<ChannelData, 10000u, true, true, false, false> >;
}