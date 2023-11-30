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
        enum Type
        {
            ServerSocket,
            DataSocket,
            Timer,
            Pipe
        };

        IFDEventHandler(const IFDEventHandler&)               = delete;
        IFDEventHandler& operator=(const IFDEventHandler&)    = delete;
        IFDEventHandler(IFDEventHandler&& other) = default;
        IFDEventHandler& operator=(IFDEventHandler&& other) = default;

        virtual bool onEvent(struct ::epoll_event& event) = 0;
        virtual const FileDescriptor& getFD() const = 0;
        virtual ~IFDEventHandler() {}

        Type getType() const { return m_type; }

    protected:
        IFDEventHandler(Type type):m_type(type) {}

    private:
        Type m_type;
    };
}