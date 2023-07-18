
#include <platform/FileDescriptor.h>

#include <unistd.h>

using namespace lu::platform;


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

FileDescriptor::~FileDescriptor()
{
    if (m_fd == NULL_FD)
    {
        return;
    }

    ::close(m_fd);
}
