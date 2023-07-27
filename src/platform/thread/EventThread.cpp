#include <platform/thread/EventThread.h>
#include <platform/thread/IEventThreadCallback.h>
#include <glog/logging.h>

using namespace lu::platform::thread;

template<lu::common::NonPtrClassOrStruct EventThreadCallback>
EventThread<EventThreadCallback>::EventThread(EventThreadCallback& severEventThreadCallback,
    const std::string& name, EventThreadConfig serverConfig)
    :
    m_name(name),
    m_severEventThreadCallback(severEventThreadCallback),
    m_clientThreadConfig(serverConfig),
    m_eventLoop(),
    m_timer(m_severEventThreadCallback, serverConfig.TIMER_NAME),
    m_thread()
{

}

template<lu::common::NonPtrClassOrStruct EventThreadCallback>
EventThread<EventThreadCallback>::EventThread(EventThread&& other):
    m_name(std::move(other.m_name)),
    m_severEventThreadCallback(other.m_severEventThreadCallback),
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
    m_severEventThreadCallback = std::move(other.m_severEventThreadCallback);
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

    m_severEventThreadCallback.onInit();

    return true;
}

template<lu::common::NonPtrClassOrStruct EventThreadCallback>
void EventThread<EventThreadCallback>::run()
{
    LOG(INFO) << "Started " << m_name;
    m_severEventThreadCallback.onStart();

    if (m_clientThreadConfig.TIMER_IN_MSEC != 0u)
    {
        m_timer.setToNonBlocking();
        m_eventLoop.add(m_timer);
        m_timer.start(0, (int) m_clientThreadConfig.TIMER_IN_MSEC * 1'000'1000);
    }

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
    m_severEventThreadCallback.onExit();
}

template class EventThread<IEventThreadCallback>;