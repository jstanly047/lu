
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
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
        int fdFlags = ::fcntl(fileDescriptorAndFlags.fileDescriptor, F_GETFL);

        if (fdFlags == FLAG_SET_OR_GET_FAILED) 
        {
            return false;
        }

        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        fdFlags |= fileDescriptorAndFlags.flags;

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
        return ::fcntl(fileDescriptorAndFlags.fileDescriptor, F_SETFL, fdFlags) != FLAG_SET_OR_GET_FAILED;
    }

    bool removeFDFlags(FileDescriptorAndFlags fileDescriptorAndFlags)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
        int fdFlags = ::fcntl(fileDescriptorAndFlags.fileDescriptor, F_GETFL);

        if (fdFlags == FLAG_SET_OR_GET_FAILED) 
        {
            return false;
        }

        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        fdFlags &= ~fileDescriptorAndFlags.flags;

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
        return ::fcntl(fileDescriptorAndFlags.fileDescriptor, F_SETFL, fdFlags) != FLAG_SET_OR_GET_FAILED;
    }
}

FileDescriptor::FileDescriptor(int fileDescriptor) : m_fd(fileDescriptor) {}

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

bool FileDescriptor::setToNonBlocking() const
{
    return setFDFlags({m_fd, O_NONBLOCK});
}

bool FileDescriptor::setBlocking() const
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
