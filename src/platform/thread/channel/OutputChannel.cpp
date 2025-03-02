#include <platform/thread/channel/OutputChannel.h>
#include <queue/Queue.h>

namespace
{
    thread_local const lu::platform::thread::channel::ChannelID channelID =  std::hash<std::thread::id>{}(std::this_thread::get_id());
}

using namespace lu::platform::thread::channel;

void OutputChannel::transferMsg(unsigned int threadIdx, void *msg)
{
    m_transferQueues[threadIdx]->push({channelID, msg});
}

void OutputChannel::transferMsg(const std::string& threadName, void *msg)
{
    m_transferQueues[getThreadIndx(threadName)]->push({channelID, msg});
}

void OutputChannel::add(const std::string &threadName, TransferQueue &transferQueue)
{
    m_transferQueues.push_back(&transferQueue);
    auto itr = m_threadNameIndexMap.insert({threadName, m_transferQueues.size() - 1});
    // TODO Enable this even in Release build
    assert(itr.second);
}

unsigned int OutputChannel::getThreadIndx(const std::string &threadName) const
{
    auto itr = m_threadNameIndexMap.find(threadName);
    assert(itr != m_threadNameIndexMap.end());
    return itr->second;
}

ChannelID OutputChannel::getChannelID() 
{
    return channelID;
}