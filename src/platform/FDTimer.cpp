#include <platform/FDTimer.h>
#include <platform/ITimerHandler.h>
#include <glog/logging.h>

#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <cassert>

using namespace lu::platform;

namespace
{
    lu::platform::FileDescriptor nullFileDescriptor(nullptr);
}

template<lu::common::NonPtrClassOrStruct TimerCallback>
FDTimer<TimerCallback>::FDTimer(TimerCallback& timerCallback) : m_timerCallback(timerCallback)
{
    int fd = ::timerfd_create(CLOCK_MONOTONIC, 0);
    
    if (fd == NULL_FD) 
    {
        LOG(ERROR) << "Failed to create timer fd!";
    }

    m_fd.reset(new FileDescriptor(fd));
}

template<lu::common::NonPtrClassOrStruct TimerCallback>
FDTimer<TimerCallback>::FDTimer(FDTimer&& other) noexcept :
    m_fd(std::move(other.m_fd)),
    m_interval(std::move(other.m_interval)),
    m_timerCallback(other.m_timerCallback)
{
}

template<lu::common::NonPtrClassOrStruct TimerCallback>
FDTimer<TimerCallback>& FDTimer<TimerCallback>::operator=(FDTimer<TimerCallback>&& other) noexcept
{
    m_fd = std::move(other.m_fd);
    m_interval = other.m_interval;
    m_timerCallback = other.m_timerCallback;
    return *this;
}

template<lu::common::NonPtrClassOrStruct TimerCallback>
bool FDTimer<TimerCallback>::start(int intervalInSec, int interValInNonSec, bool repeat)
{
    assert(*m_fd != nullptr);
    m_interval.it_value.tv_sec = intervalInSec;
    m_interval.it_value.tv_nsec = interValInNonSec;

    if (repeat)
    {
        m_interval.it_interval.tv_sec = intervalInSec;
        m_interval.it_interval.tv_nsec = interValInNonSec;
    }
    else
    {
        m_interval.it_interval.tv_sec = 0;
        m_interval.it_interval.tv_nsec = 0;
    }

    if (::timerfd_settime(*m_fd, 0, &m_interval, nullptr) == -1) 
    {
        LOG(ERROR) << "Failed to set timer!";
        return false;
    }

    return true;
}

template<lu::common::NonPtrClassOrStruct TimerCallback>
void FDTimer<TimerCallback>::onEvent(struct ::epoll_event& event) 
{
    // TODO what are the other event can happens EPOLLHUP dont part of timer fd
    if ((event.events & EPOLLERR))
    {
        m_fd.reset(nullptr);
        return;
    }
    else if (!(event.events & EPOLLIN))
    {
        return;
    }

    int64_t res;
    if (::read(*m_fd, &res, sizeof(res)) == -1)
    {
        LOG(ERROR) << "Read failed for timer FD[" << (int) *m_fd << "]!";
        return;
    }
    m_timerCallback.onTimer(*this);
}

template<lu::common::NonPtrClassOrStruct TimerCallback>
const lu::platform::FileDescriptor& FDTimer<TimerCallback>::getFD() const
{
    if (m_fd == nullptr)
    {
        return nullFileDescriptor;
    }

    return *m_fd;
}

template<lu::common::NonPtrClassOrStruct TimerCallback>
bool FDTimer<TimerCallback>::stop()
{
    m_interval.it_value.tv_sec = 0;
    m_interval.it_value.tv_nsec = 0;
    m_interval.it_interval.tv_sec = 0;
    m_interval.it_interval.tv_nsec = 0;

    if (::timerfd_settime(*m_fd, 0, &m_interval, nullptr) == -1) 
    {
        LOG(ERROR) << "Failed to stop the timer!";
        return false;
    }

    return true;
}

template<lu::common::NonPtrClassOrStruct TimerCallback>
void FDTimer<TimerCallback>::setToNonBlocking()
{
    if (m_fd->setToNonBlocking() == false)
    {
        LOG(ERROR) << "Can not set non blocking for timer fd " << (int) *m_fd << "!";
    }
}


template class lu::platform::FDTimer<lu::platform::ITimerHandler>;