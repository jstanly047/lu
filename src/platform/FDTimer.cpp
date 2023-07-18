#include <platform/FDTimer.h>
#include <glog/logging.h>

#include <sys/timerfd.h>
#include <unistd.h>
#include <cassert>

using namespace lu::platform;

template<lu::common::NonPtrClassOrStruct TimerCallback>
FDTimer<TimerCallback>::FDTimer()
{
    int fd = ::timerfd_create(CLOCK_MONOTONIC, 0);
    
    if (fd == NULL_FD) 
    {
        LOG(ERROR) << "Failed to create timer file descriptor!";
    }

    m_fd = new FileDescriptor(fd);
}

template<lu::common::NonPtrClassOrStruct TimerCallback>
FDTimer<TimerCallback>::FDTimer(FDTimer&& other) noexcept :
    m_fd(std::move(other.m_fd)),
    m_interval(std::move(other.m_interval))
{
}

template<lu::common::NonPtrClassOrStruct TimerCallback>
FDTimer<TimerCallback>& FDTimer<TimerCallback>::operator=(FDTimer<TimerCallback>&& other) noexcept
{
    m_fd = std::move(other.m_fd);
    m_interval = other.m_interval;
}

template<lu::common::NonPtrClassOrStruct TimerCallback>
bool FDTimer<TimerCallback>::start(int intervalInSec, int interValInNonSec)
{
    assert(*m_fd == nullptr);
    m_interval.it_interval.tv_sec = intervalInSec;
    m_interval.it_interval.tv_nsec = interValInNonSec;
    m_interval.it_value = m_interval.it_interval;

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
    m_timerCallback.onTimer(*this);

    if (m_stop == false)
    {
        return;
    }

    itimerspec stopSpec{};
    if (::timerfd_settime(*m_fd, 0, &stopSpec, nullptr) == -1) 
    {
        LOG(ERROR) << "Failed stop timer!";
        return;
    }

    m_fd = new FileDescriptor(NULL_FD);
}