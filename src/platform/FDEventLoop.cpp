#include <platform/FDEventLoop.h>
#include <common/Defs.h>
#include <platform/Defs.h>
#include <glog/logging.h>

#include <cassert>
#include <sys/epoll.h>
#include <vector>

using namespace lu::platform;

FDEventLoop::~FDEventLoop()
{
    LOG(INFO) << "Destroy FD[" << m_epollFD << "] event pool!";
    close();
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
    if (!event.getFD().setToNonBlocking())
    {
        LOG(ERROR) << "FD[" << event.getFD() << "] fail to set non blocking!!";
        return false;
    }

    struct epoll_event epollEvent{};
    epollEvent.events = EPOLLIN | EPOLLET;
    epollEvent.data.ptr = &event;
    return ::epoll_ctl(m_epollFD, EPOLL_CTL_ADD, event.getFD(), &epollEvent) == 0;
}

bool FDEventLoop::add(std::unique_ptr<IFDEventHandler>&& event) 
{
    if (add(*event.get()) == false)
    {
        return false;
    }

    return m_IFDEventHandlers.insert(std::move(event)).second;
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
            auto iFDEventHandler = (reinterpret_cast<IFDEventHandler*>(events[i].data.ptr));

            if (iFDEventHandler->onEvent(events[i]) == false)
            {
                ::epoll_ctl(m_epollFD, EPOLL_CTL_DEL, events[i].data.fd, nullptr);

                if (iFDEventHandler->getType() == IFDEventHandler::DataSocket)
                {
                    auto itr= m_IFDEventHandlers.find(iFDEventHandler);
                    m_IFDEventHandlers.erase(itr);
                    continue;
                }
            }
        }

        if (UNLIKELY(m_stop))
        {
            close();
            LOG(INFO) << "Stoped FD[" << m_epollFD << "] event pool!";
            break;
        }
    }
}

void FDEventLoop::stop()
{
    /*If a file descriptor being monitored by select() is closed in another thread, the result is unspecified.*/
    m_stop = true;
}

void FDEventLoop::close()
{
    if (m_epollFD == -1)
    {
        return;
    }
    
    if (::close(m_epollFD) == -1)
    {
        LOG(ERROR) << "Try stop event FD[" << m_epollFD << "] failed!";
        return;
    }

    m_epollFD = lu::platform::NULL_FD;
}