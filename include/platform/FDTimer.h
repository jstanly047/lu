#pragma once

#include <platform/IFDEventHandler.h>
#include <common/TemplateConstraints.h>
#include <platform/defs.h>

#include <time.h>

namespace lu::platform
{
    template<lu::common::NonPtrClassOrStruct TimerCallback> 
    class FDTimer : public IFDEventHandler
    {
    public:
        FDTimer(const FDTimer&)               = delete;
        FDTimer& operator=(const FDTimer&)    = delete;

        FDTimer(FDTimer&& other) noexcept;
        FDTimer& operator=(FDTimer&& other) noexcept;

        bool start(int intervalInSec, int interValInNonSec);
        void stop() { m_stop = true; }

    private:
        void onEvent(struct ::epoll_event& event) override final;
        int getFD() const { return m_fd; }

        int m_fd = lu::platform::NULL_FD;
        itimerspec m_interval{};
        TimerCallback& m_timerCallback;
        bool m_stop = false;
    };
}