#pragma once

#include <platform/defs.h>

#include <cstddef>

namespace lu::platform
{
    class FileDescriptor
    {
    public:
        FileDescriptor(const FileDescriptor&)               = delete;
        FileDescriptor& operator=(const FileDescriptor&)    = delete;
        FileDescriptor(FileDescriptor&& other) = delete;;
        FileDescriptor& operator=(FileDescriptor&& other) = delete;

        FileDescriptor(int fd);
        FileDescriptor(std::nullptr_t);
        ~FileDescriptor();

        operator int() const;
        bool operator ==(const FileDescriptor &other) const;
        bool operator !=(const FileDescriptor &other) const;
        bool operator ==(std::nullptr_t) const;
        bool operator !=(std::nullptr_t) const;

        bool setToNonBlocking();
        bool setBlocking();

    private:
        int m_fd;
    };
}