
#include <platform/FileDescriptor.h>

#include <unistd.h>
#include <fcntl.h>

using namespace lu::platform;

namespace 
{
    bool setFDFlags(int fd, int Flags)
    {
        int fdFlags = fcntl(fd, F_GETFL, 0);

        if (fdFlags == -1) 
        {
            return false;
        }

        fdFlags |= Flags;

        if (fcntl(fd, F_SETFL, fdFlags) == -1) 
        {
            return false;
        }

        return true;
    }

    bool removeFDFlags(int fd, int Flags)
    {
        int fdFlags = fcntl(fd, F_GETFL, 0);

        if (fdFlags == -1) 
        {
            return false;
        }

        fdFlags &= ~Flags;

        if (fcntl(fd, F_SETFL, fdFlags) == -1) 
        {
            return false;
        }

        return true;
    }
}

FileDescriptor::FileDescriptor(int fd) : m_fd(fd) {}

FileDescriptor::FileDescriptor(std::nullptr_t) : m_fd(NULL_FD) {}

FileDescriptor::operator int() const
{
    return m_fd;
}

bool FileDescriptor::operator ==(const FileDescriptor &other) const 
{
    return m_fd == other.m_fd;
}

bool FileDescriptor::operator !=(const FileDescriptor &other) const 
{
    return m_fd != other.m_fd;
}

bool FileDescriptor::operator ==(std::nullptr_t) const 
{
    return m_fd == NULL_FD;
}

bool FileDescriptor::operator !=(std::nullptr_t) const 
{
    return m_fd != NULL_FD;
}

bool FileDescriptor::setToNonBlocking()
{
    return setFDFlags(m_fd, O_NONBLOCK);
}

bool FileDescriptor::setBlocking()
{
    return removeFDFlags(m_fd, O_NONBLOCK);
}

FileDescriptor::~FileDescriptor()
{
    if (m_fd == NULL_FD)
    {
        return;
    }

    ::close(m_fd);
}
