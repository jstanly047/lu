#pragma once

#include <platform/IFDEventHandler.h>
#include <common/TemplateConstraints.h>
#include <platform/defs.h>

#include <time.h>

#include <memory>
#include <string>
#include <glog/logging.h>

#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <cassert>


namespace lu::platform
{
    template<lu::common::NonPtrClassOrStruct TimerCallback> 
    class FDTimer : public IFDEventHandler
    {
    public:
        FDTimer(const FDTimer &) = delete;
        FDTimer &operator=(const FDTimer &) = delete;

        FDTimer(FDTimer &&other) noexcept: 
                IFDEventHandler(std::move(other)),
                m_fd(std::move(other.m_fd)),
                m_interval(std::move(other.m_interval)),
                m_timerCallback(other.m_timerCallback),
                m_name(std::move(other.m_name)),
                m_stop(std::move(other.m_stop))
        {
        }

        FDTimer &operator=(FDTimer &&other) noexcept
        {
            IFDEventHandler::operator=(std::move(other));
            m_fd = std::move(other.m_fd);
            m_interval = other.m_interval;
            m_timerCallback = other.m_timerCallback;
            m_name = std::move(other.m_name);
            m_stop = std::move(other.m_stop);
            return *this;
        }

        FDTimer(TimerCallback &timerCallback, const std::string &name) : IFDEventHandler(IFDEventHandler::Timer),
                                                                         m_fd(),
                                                                         m_interval(),
                                                                         m_timerCallback(timerCallback),
                                                                         m_name(name),
                                                                         m_stop(false)
        {
        }

        bool init()
        {
            int fd = ::timerfd_create(CLOCK_MONOTONIC, 0);

            if (fd == NULL_FD)
            {
                return false;
            }

            m_fd.reset(new FileDescriptor(fd));
            return true;
        }

        bool start(int intervalInSec, int interValInNonSec, bool repeat = true)
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
                return false;
            }

            return true;
        }

        bool stop()
        {
            m_interval.it_value.tv_sec = 0;
            m_interval.it_value.tv_nsec = 0;
            m_interval.it_interval.tv_sec = 0;
            m_interval.it_interval.tv_nsec = 0;

            if (::timerfd_settime(*m_fd, 0, &m_interval, nullptr) == -1)
            {
                return false;
            }

            return true;
        }

        bool setToNonBlocking()
        {
            return m_fd->setToNonBlocking();
        }

        const FileDescriptor &getFD() const
        {
            static FileDescriptor nullFileDescriptor(nullptr);

            if (m_fd == nullptr)
            {
                return nullFileDescriptor;
            }

            return *m_fd;
        }

        const auto &getName() const { return m_name; }
        virtual ~FDTimer() {}

    private:
        bool onEvent(struct ::epoll_event &event) override final
        {
            // TODO what are the other event can happens EPOLLHUP dont part of timer fd
            if ((event.events & EPOLLERR))
            {
                m_fd.reset(nullptr);
                return true;
            }
            else if (!(event.events & EPOLLIN))
            {
                return true;
            }

            int64_t res;
            if (::read(*m_fd, &res, sizeof(res)) == -1)
            {
                LOG(ERROR) << "Read failed for timer FD[" << (int)*m_fd << "]!";
                return true;
            }

            m_timerCallback.onTimer(*this);
            return true;
        }

        std::unique_ptr<FileDescriptor> m_fd;
        itimerspec m_interval{};
        TimerCallback &m_timerCallback;
        std::string m_name;
        bool m_stop = false;
    };
}