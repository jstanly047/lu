#include <platform/EventChannel.h>
#include <platform/socket/BaseSocket.h>
#include <platform/thread/ClientThread.h>
#include <platform/thread/IClientThreadCallback.h>
#include <platform/socket/data_handler/String.h>
#include <glog/logging.h>

#include <unistd.h>
#include <sys/epoll.h>

using namespace lu::platform;

namespace
{
    lu::platform::FileDescriptor nullFileDescriptor(nullptr);
}

template<lu::common::NonPtrClassOrStruct EventChannelHandler>
EventChannel<EventChannelHandler>::EventChannel(EventChannelHandler& eventChannelHandler): m_eventChannelHandler(eventChannelHandler)
{

}


template<lu::common::NonPtrClassOrStruct EventChannelHandler>
EventChannel<EventChannelHandler>::EventChannel(EventChannel<EventChannelHandler>&& other):
    m_eventChannelHandler(other.m_eventChannelHandler),
    m_in(std::move(other.m_in)),
    m_out(std::move(other.m_out))
{

}

template<lu::common::NonPtrClassOrStruct EventChannelHandler>
EventChannel<EventChannelHandler>& EventChannel<EventChannelHandler>::operator=(EventChannel<EventChannelHandler>&& other)
{
    m_eventChannelHandler = std::move(other.m_eventChannelHandler);
    m_in = std::move(other.m_in);
    m_out = std::move(other.m_out);
    return *this;
}

template<lu::common::NonPtrClassOrStruct EventChannelHandler>
bool EventChannel<EventChannelHandler>::init()
{
    int pipeFDs[2];
    // TODO make it non blocking
    if (::pipe(pipeFDs) == -1)
    {
        return false;
    }

    m_in.reset(new FileDescriptor(pipeFDs[0]));
    m_out.reset(new FileDescriptor(pipeFDs[1]));
    LOG(INFO) << "File created channel out[" << (int) *m_in << "], out[" << (int) *m_out << "]";
    return true;
}

template<lu::common::NonPtrClassOrStruct EventChannelHandler>
bool EventChannel<EventChannelHandler>::notify(const EventData& eventData) const
{
    return ::write(*m_out, &eventData, sizeof(eventData));
}

template<lu::common::NonPtrClassOrStruct EventChannelHandler>
void EventChannel<EventChannelHandler>::onEvent(struct ::epoll_event& event)
{
    if ((event.events & EPOLLERR))
    {
        m_in.reset(nullptr);
        return;
    }
    else if (!(event.events & EPOLLIN))
    {
        return;
    }

    // TODO make it non blocking
    EventData eventData;
    ::read(*m_in, &eventData, sizeof(eventData));

    switch (eventData.eventType)
    {
    case EventData::NewConnection:
        m_eventChannelHandler.onNewConnection(reinterpret_cast<lu::platform::socket::BaseSocket*>(eventData.data));
        break;
    default:
        break;
    }
}

template<lu::common::NonPtrClassOrStruct EventChannelHandler>
const lu::platform::FileDescriptor& EventChannel<EventChannelHandler>::getInFD() const
{
    return getFD();
}

template<lu::common::NonPtrClassOrStruct EventChannelHandler>
const lu::platform::FileDescriptor& EventChannel<EventChannelHandler>::getOutFD() const
{
    if (m_out == nullptr)
    {
        return nullFileDescriptor;
    }

    return *m_out;
}

template<lu::common::NonPtrClassOrStruct EventChannelHandler>
const lu::platform::FileDescriptor& EventChannel<EventChannelHandler>::getFD() const
{
    if (m_in == nullptr)
    {
        return nullFileDescriptor;
    }

    return *m_in;
}

template class EventChannel<lu::platform::thread::ClientThread<lu::platform::thread::IClientThreadCallback, lu::platform::socket::data_handler::String> >;