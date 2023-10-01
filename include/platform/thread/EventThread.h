#pragma once

#include <platform/thread/LuThread.h>
#include <platform/thread/Config.h>
#include <platform/FDEventLoop.h>
#include <platform/EventChannel.h>
#include <platform/FDTimer.h>

#include <unordered_map>
#include <thread>
#include <glog/logging.h>

namespace lu::platform::thread
{
    template<lu::common::NonPtrClassOrStruct EventThreadCallback>
    class EventThread : public LuThread
    {
    public:
        EventThread(const EventThread&) = delete;
        EventThread& operator=(const EventThread&) = delete;
        EventThread(EventThread&& other) = delete ;
        EventThread& operator=(EventThread&& other)= delete;

        bool init()
        {
            LuThread::init();
            if (m_eventLoop.init() == false)
            {
                LOG(ERROR) << "[" << m_name << "] Failed init FD event loop!";
                return false;
            }

            LOG(INFO) << "[" << m_name << "] Event Loop init [" << m_eventLoop.getFD() << "]";

            if (m_clientThreadConfig.TIMER_IN_MSEC != 0u)
            {
                if (m_timer.init() == false)
                {
                    LOG(ERROR) << "[" << m_name << "] Failed to init timer!";
                    return false;
                }

                LOG(INFO) << "[" << m_name << "] Timer init [" << m_timer.getFD() << "]";
            }

            return m_eventThreadCallback.onInit();
        }

        void run()
        {
            LuThread::run();

            if (m_clientThreadConfig.TIMER_IN_MSEC != 0u)
            {
                m_timer.setToNonBlocking();
                m_eventLoop.add(m_timer);
                int sec = m_clientThreadConfig.TIMER_IN_MSEC / 1'000;
                int mSec = m_clientThreadConfig.TIMER_IN_MSEC % 1'000;
                m_timer.start(sec, mSec * 1'000'000);
                LOG(INFO) << "[" << m_name << "] Timer start [" << m_timer.getFD() << "]";
            }

            m_eventThreadCallback.onStart();
            LOG(INFO) << "[" << m_name << "] Event Loop start [" << m_eventLoop.getFD() << "]";
            m_eventLoop.start(m_clientThreadConfig.NUMBER_OF_EVENTS_PER_HANDLE);
        }

        void stop()
        {
            LuThread::stop();
            if (m_clientThreadConfig.TIMER_IN_MSEC == 0u)
            {
                m_timer.init();
                m_timer.setToNonBlocking();
                m_eventLoop.add(m_timer);
                m_timer.start(0, 1'000'1000u, false);
                LOG(INFO) << "[" << m_name << "] Timer start to stop [" << m_timer.getFD() << "]";
            }

            LOG(INFO) << "[" << m_name << "] Event Loop stop [" << m_eventLoop.getFD() << "]";
            m_eventLoop.stop(); // Expect to stop in next timer
        }
        void join()
        {
            LuThread::join();
            m_eventThreadCallback.onExit();
        }

        const std::string& getName() const { return m_name; }
        EventThreadCallback& getEventThreadCallback() { return m_eventThreadCallback; }

    protected:
        EventThread(EventThreadCallback &severEventThreadCallback, const std::string &name, EventThreadConfig clientThreadConfig)
            : LuThread(name),
              m_eventThreadCallback(severEventThreadCallback),
              m_clientThreadConfig(clientThreadConfig),
              m_eventLoop(),
              m_timer(m_eventThreadCallback, clientThreadConfig.TIMER_NAME)
        {
        }

        virtual ~EventThread() {}

        EventThreadCallback& m_eventThreadCallback;
        EventThreadConfig m_clientThreadConfig;
        lu::platform::FDEventLoop m_eventLoop;
        lu::platform::FDTimer<EventThreadCallback> m_timer;
    };
}