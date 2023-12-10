#pragma once

#include <platform/IFDEventHandler.h>
#include <platform/Defs.h>
#include <common/PointerComparator.h>
#include <set>

namespace lu::platform
{
    class FDEventLoop
    {
    public:
        FDEventLoop(const FDEventLoop&) = delete;
        FDEventLoop& operator=(const FDEventLoop&) = delete;

        FDEventLoop(FDEventLoop&& other) = delete;
        FDEventLoop& operator=(FDEventLoop&& other) = delete;
        FDEventLoop() : m_epollFD(NULL_FD), m_stop(false) {}
        ~FDEventLoop();

        bool init();
        bool add(IFDEventHandler &event) const;
        bool remove(IFDEventHandler &event) const;
        bool add(std::unique_ptr<IFDEventHandler>&& event);
        void start(int maxEvents);
        void stop();

        int getFD() { return m_epollFD; }
    private:
        void close();
        int m_epollFD = NULL_FD;
        bool m_stop{};
        std::set<std::unique_ptr<IFDEventHandler>, lu::common::PointerComparator<IFDEventHandler>> m_IFDEventHandlers{};
    };
}