#include <platform/FDEventLoop.h>
#include <platform/defs.h>
#include <glog/logging.h>

#include <sys/epoll.h>
#include <assert.h>
#include <vector>

using namespace lu::platform;

bool FDEventLoop::init()
{
    m_epollFD = ::epoll_create1(0);

    if (m_epollFD < 0) 
    {
        LOG(ERROR) << "Cannot create non FD event pool!";
        return false;
    }

    return true;
}

bool FDEventLoop::registerFDEventHandler(IFDEventHandler &event)
{
    assert(event.getFD() != nullptr);
    struct epoll_event epollEvent;
    epollEvent.events = EPOLLIN | EPOLLET;
    epollEvent.data.ptr = &event;
    return ::epoll_ctl(m_epollFD, EPOLL_CTL_ADD, event.getFD(), &epollEvent) == 0;
}

void FDEventLoop::start(int maxEvents)
{
    std::vector<struct epoll_event> events;
    events.reserve(maxEvents);

    for(;;) 
    {
        int n = ::epoll_wait(m_epollFD, events.data(), maxEvents, -1);

        for (int i = 0; i < n; ++i) 
        {
            (reinterpret_cast<IFDEventHandler*>(events[i].data.ptr))->onEvent(events[i]);
        }
    }
}