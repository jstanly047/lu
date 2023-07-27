#pragma once
#include <platform/thread/Config.h>
#include <platform/FDEventLoop.h>
#include <platform/EventChannel.h>
#include <platform/FDTimer.h>

#include <unordered_map>
#include <thread>

namespace lu::platform::thread
{
    

    template<lu::common::NonPtrClassOrStruct EventThreadCallback>
    class EventThread
    {
    public:
        EventThread(const EventThread&) = delete;
        EventThread& operator=(const EventThread&) = delete;
        EventThread(EventThread&& other);
        EventThread& operator=(EventThread&& other);
        
        bool init();
        void run();
        void stop();
        void join();

        const std::string& getName() const { return m_name; }
        EventThreadCallback& getEventThreadCallback() { return m_severEventThreadCallback; }

    protected:
        EventThread(EventThreadCallback& severEventThreadCallback, const std::string& name, EventThreadConfig clientThreadConfig);
        virtual ~EventThread() {}

        std::string m_name;
        EventThreadCallback& m_severEventThreadCallback;
        EventThreadConfig m_clientThreadConfig;
        lu::platform::FDEventLoop m_eventLoop;
        lu::platform::FDTimer<EventThreadCallback> m_timer;
        std::thread m_thread;
    };
}