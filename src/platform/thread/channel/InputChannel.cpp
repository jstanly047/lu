#include <platform/thread/channel/InputChannel.h>
#include <queue/Queue.h>

using namespace lu::platform::thread::channel;

InputChannel::InputChannel() : m_transferQueue(new TransferQueue())
{
    
}

InputChannel::~InputChannel()
{
    delete m_transferQueue;
}

void InputChannel::push(ChannelData data)
{
    m_transferQueue->push(data);
}

ChannelData InputChannel::pop()
{
    return m_transferQueue->pop();
}