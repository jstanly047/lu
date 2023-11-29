#pragma once

#include <platform/FileDescriptor.h>

struct epoll_event;

/*
 TODO : we can try using function pointer to check the performance
*/

namespace lu::platform
{
    class IFDEventHandler
    {
    public:
        IFDEventHandler(const IFDEventHandler&)               = delete;
        IFDEventHandler& operator=(const IFDEventHandler&)    = delete;
        IFDEventHandler(IFDEventHandler&& other) = delete;
        IFDEventHandler& operator=(IFDEventHandler&& other) = delete;

        virtual bool onEvent(struct ::epoll_event& event) = 0;
        virtual const FileDescriptor& getFD() const = 0;
        virtual ~IFDEventHandler() {}

    protected:
        IFDEventHandler() {}
    };
}