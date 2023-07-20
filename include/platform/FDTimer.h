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
        FDTimer(TimerCallback& timerCallback);

        bool start(int intervalInSec, int interValInNonSec, bool repeat=true);
        bool stop();
        void setToNonBlocking();

        const FileDescriptor& getFD() const;
        virtual ~FDTimer() {}

    private:
        void onEvent(struct ::epoll_event& event) override final;
        

        std::unique_ptr<FileDescriptor> m_fd;
        itimerspec m_interval{};
        TimerCallback& m_timerCallback;
        bool m_stop = false;
    };
}