#include <platform/thread/EventThread.h>
#include <platform/thread/IEventThreadCallback.h>
#include <platform/thread/IServerThreadCallback.h>
#include <platform/thread/IClientThreadCallback.h>
#include <platform/thread/IConnectionThreadCallback.h>

#include <glog/logging.h>

using namespace lu::platform::thread;

template<lu::common::NonPtrClassOrStruct EventThreadCallback>
EventThread<EventThreadCallback>::EventThread(EventThreadCallback& severEventThreadCallback,
    const std::string& name, EventThreadConfig serverConfig)
    :
    m_name(name),
    m_eventThreadCallback(severEventThreadCallback),
    m_clientThreadConfig(serverConfig),
    m_eventLoop(),
    m_timer(m_eventThreadCallback, serverConfig.TIMER_NAME),
    m_thread()
{

}

template<lu::common::NonPtrClassOrStruct EventThreadCallback>
EventThread<EventThreadCallback>::EventThread(EventThread&& other):
    m_name(std::move(other.m_name)),
    m_eventThreadCallback(other.m_eventThreadCallback),
    m_clientThreadConfig(std::move(other.m_clientThreadConfig)),
    m_eventLoop(std::move(other.m_eventLoop)),
    m_timer(std::move(other.m_timer)),
    m_thread(std::move(other.m_thread))
    
{

}

template<lu::common::NonPtrClassOrStruct EventThreadCallback>
EventThread<EventThreadCallback>& EventThread<EventThreadCallback>::operator=(EventThread<EventThreadCallback>&& other)
{
    m_name = std::move(other.m_name);
    m_eventThreadCallback = std::move(other.m_eventThreadCallback);
    m_clientThreadConfig = other.m_clientThreadConfig;
    m_eventLoop = std::move(other.m_eventLoop);
    m_timer = std::move(other.m_timer);
    m_thread = std::move(other.m_thread);
    return *this;
}

template<lu::common::NonPtrClassOrStruct EventThreadCallback>
bool EventThread<EventThreadCallback>::init()
{
    if (m_eventLoop.init() == false)
    {
        LOG(ERROR) << "Thead[" << m_name << "] failed init FD event loop!";
        return false;
    }

    if (m_clientThreadConfig.TIMER_IN_MSEC != 0u)
    {
        if (m_timer.init() == false)
        {
            LOG(ERROR) << "Thread[" << m_name << "] failed to create timer!";
            return false;
        }
    }

    return m_eventThreadCallback.onInit();
}

template<lu::common::NonPtrClassOrStruct EventThreadCallback>
void EventThread<EventThreadCallback>::run()
{
    LOG(INFO) << "Started " << m_name;
    if (m_clientThreadConfig.TIMER_IN_MSEC != 0u)
    {
        m_timer.setToNonBlocking();
        m_eventLoop.add(m_timer);
        int sec = m_clientThreadConfig.TIMER_IN_MSEC / 1'000;
        int mSec = m_clientThreadConfig.TIMER_IN_MSEC % 1'000;
        m_timer.start(sec, mSec * 1'000'000);
    }

    m_eventThreadCallback.onStart();
    m_eventLoop.start(m_clientThreadConfig.NUMBER_OF_EVENTS_PER_HANDLE);
}

template<lu::common::NonPtrClassOrStruct EventThreadCallback>
void EventThread<EventThreadCallback>::stop()
{
    LOG(INFO) << "Stopping " << m_name;

    if (m_clientThreadConfig.TIMER_IN_MSEC == 0u)
    {
        m_timer.init();
        m_timer.setToNonBlocking();
        m_eventLoop.add(m_timer);
        m_timer.start(0, 1'000'1000u, false);
    }

    m_eventLoop.stop(); //Expect to stop in next timer
}

template<lu::common::NonPtrClassOrStruct EventThreadCallback>
void EventThread<EventThreadCallback>::join()
{
    m_thread.join();
    m_eventThreadCallback.onExit();
}

template class EventThread<IEventThreadCallback>;
template class EventThread<IServerThreadCallback>;
template class EventThread<IClientThreadCallback>;
template class EventThread<IConnectionThreadCallback>;