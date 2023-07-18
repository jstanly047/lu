#pragma once

#include <platform/IFDEventHandler.h>
#include <common/TemplateConstraints.h>
#include <platform/defs.h>

#include <time.h>

#include <memory>

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
        FDTimer();

        bool start(int intervalInSec, int interValInNonSec);
        void stop() { m_stop = true; }

    private:
        void onEvent(struct ::epoll_event& event) override final;
        const FileDescriptor& getFD() const { return *m_fd; }

        std::unique_ptr<FileDescriptor> m_fd;
        itimerspec m_interval{};
        TimerCallback& m_timerCallback;
        bool m_stop = false;
    };
}