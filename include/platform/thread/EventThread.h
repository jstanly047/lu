#pragma once

#include <platform/thread/LuThread.h>
#include <platform/thread/Config.h>
#include <platform/FDEventLoop.h>
#include <platform/EventChannel.h>
#include <platform/FDTimer.h>

#include <unordered_map>
#include <thread>

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
        
        bool init();
        void run();
        void stop();
        void join();

        const std::string& getName() const { return m_name; }
        EventThreadCallback& getEventThreadCallback() { return m_eventThreadCallback; }

    protected:
        EventThread(EventThreadCallback& severEventThreadCallback, const std::string& name, EventThreadConfig clientThreadConfig);
        virtual ~EventThread() {}

        EventThreadCallback& m_eventThreadCallback;
        EventThreadConfig m_clientThreadConfig;
        lu::platform::FDEventLoop m_eventLoop;
        lu::platform::FDTimer<EventThreadCallback> m_timer;
    };
}