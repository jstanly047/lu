#include <platform/FDEventLoop.h>
#include <common/defs.h>
#include <platform/defs.h>
#include <glog/logging.h>

#include <cassert>
#include <sys/epoll.h>
#include <vector>

using namespace lu::platform;

FDEventLoop::FDEventLoop(FDEventLoop&& other) noexcept {
    std::swap(m_epollFD, other.m_epollFD);
    std::swap(m_stop, other.m_stop);
}

FDEventLoop& FDEventLoop::operator=(FDEventLoop&& other)noexcept 
{
    std::swap(m_epollFD, other.m_epollFD);
    std::swap(m_stop, other.m_stop);
    return *this;
}

FDEventLoop::~FDEventLoop()
{
    if (m_epollFD == lu::platform::NULL_FD)
    {
        return;
    }
    
    if (::close(m_epollFD) == -1)
    {
        LOG(ERROR) << "Cannot close FD[" << m_epollFD << "] event pool!";
        return;
    }

    LOG(INFO) << "Destroy FD[" << m_epollFD << "] event pool!";
}

bool FDEventLoop::init()
{
    m_epollFD = ::epoll_create1(0);

    if (m_epollFD < 0) 
    {
        LOG(ERROR) << "Cannot create FD event pool!";
        return false;
    }

    LOG(INFO) << "FD[" << m_epollFD << "] created for event pool!";
    return true;
}

bool FDEventLoop::add(IFDEventHandler &event) const
{
    assert(event.getFD() != nullptr);
    struct epoll_event epollEvent{};
    epollEvent.events = EPOLLIN | EPOLLET;
    epollEvent.data.ptr = &event;
    return ::epoll_ctl(m_epollFD, EPOLL_CTL_ADD, event.getFD(), &epollEvent) == 0;
}

bool FDEventLoop::remove(IFDEventHandler &event) const
{
    assert(event.getFD() != nullptr);
    return ::epoll_ctl(m_epollFD, EPOLL_CTL_DEL, event.getFD(), nullptr) == 0;
}

void FDEventLoop::start(int maxEvents)
{
    std::vector<struct epoll_event> events;
    events.reserve(maxEvents);

    for(;;) 
    {
        int numberEvents = ::epoll_wait(m_epollFD, events.data(), maxEvents, -1);

        for (int i = 0; i < numberEvents; ++i) 
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            (reinterpret_cast<IFDEventHandler*>(events[i].data.ptr))->onEvent(events[i]);

            if ((((events[i].events & EPOLLHUP) != 0U) || ((events[i].events & EPOLLERR) != 0U)))
            {
                ::epoll_ctl(m_epollFD, EPOLL_CTL_DEL, events[i].data.fd, nullptr);
            }
        }

        if (UNLIKELY(m_stop))
        {
            if (::close(m_epollFD) == -1)
            {
                LOG(ERROR) << "Try stop event FD[" << m_epollFD << "] failed!";
                return;
            }

            LOG(INFO) << "Stoped FD[" << m_epollFD << "] event pool!";
            m_epollFD = lu::platform::NULL_FD;
            break;
        }
    }
}

void FDEventLoop::stop()
{
    /*If a file descriptor being monitored by select() is closed in another thread, the result is unspecified.*/
    m_stop = true;
}