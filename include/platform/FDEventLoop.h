#pragma once

#include <platform/IFDEventHandler.h>
#include <platform/defs.h>

namespace lu::platform
{
    class FDEventLoop
    {
    public:
        FDEventLoop(const FDEventLoop&) = delete;
        FDEventLoop& operator=(const FDEventLoop&) = delete;

        FDEventLoop(FDEventLoop&& other);
        FDEventLoop& operator=(FDEventLoop&& other);
        FDEventLoop() : m_epollFD(NULL_FD), m_stop(false) {}
        ~FDEventLoop();

        bool init();
        bool add(IFDEventHandler &event);
        bool remove(IFDEventHandler &event);
        void start(int maxEvents);
        void stop();

        int getFD() { return m_epollFD; }
    private:
        int m_epollFD = NULL_FD;
        bool m_stop{};
    };
}