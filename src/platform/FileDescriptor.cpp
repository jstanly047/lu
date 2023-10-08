
#include <platform/FileDescriptor.h>

#include <unistd.h>
#include <fcntl.h>

using namespace lu::platform;

constexpr int FLAG_SET_OR_GET_FAILED = -1;
namespace 
{
    struct FileDescriptorAndFlags
    {
        int fileDescriptor;
        int flags;
    };
    
    bool setFDFlags(FileDescriptorAndFlags fileDescriptorAndFlags)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
        int fdFlags = ::fcntl(fileDescriptorAndFlags.fileDescriptor, F_GETFL, 0);

        if (fdFlags == FLAG_SET_OR_GET_FAILED) 
        {
            return false;
        }

        fdFlags |= fileDescriptorAndFlags.flags;

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
        if (::fcntl(fileDescriptorAndFlags.fileDescriptor, F_SETFL, fdFlags) == FLAG_SET_OR_GET_FAILED) 
        {
            return false;
        }

        return true;
    }

    bool removeFDFlags(FileDescriptorAndFlags fileDescriptorAndFlags)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
        int fdFlags = ::fcntl(fileDescriptorAndFlags.fileDescriptor, F_GETFL, 0);

        if (fdFlags == FLAG_SET_OR_GET_FAILED) 
        {
            return false;
        }

        fdFlags &= ~fileDescriptorAndFlags.flags;

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
        if (::fcntl(fileDescriptorAndFlags.fileDescriptor, F_SETFL, fdFlags) == FLAG_SET_OR_GET_FAILED) 
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
    return setFDFlags({m_fd, O_NONBLOCK});
}

bool FileDescriptor::setBlocking()
{
    return removeFDFlags({m_fd, O_NONBLOCK});
}

FileDescriptor::~FileDescriptor()
{
    if (m_fd == NULL_FD)
    {
        return;
    }

    ::close(m_fd);
}
