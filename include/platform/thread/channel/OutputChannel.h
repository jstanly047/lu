#pragma once

#include <platform/thread/channel/TransferQueue.h>
#include <thread>
#include <vector>
#include <string>
#include <map>

namespace lu::platform::thread::channel
{
    class OutputChannel
    {
    public:
        void transferMsg(unsigned int threadIdx, void* msg);
        void transferMsg(const std::string& threadName, void* msg);
        void add(const std::string& threadName, TransferQueue& transferQueue);
        unsigned int getThreadIndx(const std::string& threadName) const;
        static ChannelID getChannelID();

    private:
        std::vector<TransferQueue*> m_transferQueues;
        std::map<std::string, unsigned int> m_threadNameIndexMap;
    };
}