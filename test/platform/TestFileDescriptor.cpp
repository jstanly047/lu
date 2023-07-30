#include <platform/FileDescriptor.h>
#include <gtest/gtest.h>
#include <sys/socket.h>

TEST(TestFileDescriptor, checkNullFileDescriptor)
{
    lu::platform::FileDescriptor fileDescriptor(nullptr);
    ASSERT_EQ(fileDescriptor , nullptr);
    int fileID = fileDescriptor;
    ASSERT_TRUE(fileID == lu::platform::NULL_FD);
    lu::platform::FileDescriptor fileDescriptorSameComp(nullptr);
    ASSERT_TRUE(fileDescriptor == fileDescriptorSameComp);
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    lu::platform::FileDescriptor fileDescriptorDiffComp(fd);
    ASSERT_TRUE(fileDescriptor != fileDescriptorDiffComp);
}

TEST(TestFileDescriptor, checkFileDescriptor)
{
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    lu::platform::FileDescriptor fileDescriptor(fd);

    ASSERT_NE(fileDescriptor, nullptr);
    int fileID = fileDescriptor;
    ASSERT_TRUE(fileID == fd);
    lu::platform::FileDescriptor fileDescriptorSameComp(fd);
    ASSERT_TRUE(fileDescriptor == fileDescriptorSameComp);
    int fd2 = ::socket(AF_INET, SOCK_STREAM, 0);
    lu::platform::FileDescriptor fileDescriptorDiffComp(fd2);
    ASSERT_TRUE(fileDescriptor != fileDescriptorDiffComp);
}