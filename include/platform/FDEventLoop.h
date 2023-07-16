#pragma once

#include <platform/IFDEventHandler.h>
#include <platform/defs.h>

namespace lu::platform
{
    class FDEventLoop
    {
    public:
        FDEventLoop(FDEventLoop&& other) = delete;
        FDEventLoop& operator=(FDEventLoop&& other) = delete;
        FDEventLoop(const FDEventLoop&) = delete;
        FDEventLoop& operator=(const FDEventLoop&) = delete;

        bool init();
        bool registerFDEventHandler(IFDEventHandler &event);
        void start(int maxEvents);
        //TODO
        //void stop();

    private:
        int m_epollFD = NULL_FD;
    };
}