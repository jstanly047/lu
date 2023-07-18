#pragma once

#include <platform/FileDescriptor.h>

struct epoll_event;


namespace lu::platform
{
    class IFDEventHandler
    {
    public:
        IFDEventHandler(const IFDEventHandler&)               = delete;
        IFDEventHandler& operator=(const IFDEventHandler&)    = delete;
        IFDEventHandler(IFDEventHandler&& other) = delete;
        IFDEventHandler& operator=(IFDEventHandler&& other) = delete;

        virtual void onEvent(struct ::epoll_event& event) = 0;
        virtual const FileDescriptor& getFD() const = 0;
    
    protected:
        IFDEventHandler() {}
        ~IFDEventHandler() {}
    };
}