#include <platform/FileDescriptor.h>
#include <gtest/gtest.h>


TEST(TestFileDescriptor, checkNullFileDescriptor)
{
    lu::platform::FileDescriptor fileDescriptor(nullptr);

    ASSERT_EQ(fileDescriptor , nullptr);
    int fileID = fileDescriptor;
    ASSERT_TRUE(fileID == lu::platform::NULL_FD);
    lu::platform::FileDescriptor fileDescriptorSameComp(nullptr);
    ASSERT_TRUE(fileDescriptor == fileDescriptorSameComp);
    lu::platform::FileDescriptor fileDescriptorDiffComp(1);
    ASSERT_TRUE(fileDescriptor != fileDescriptorDiffComp);
}

TEST(TestFileDescriptor, checkFileDescriptor)
{
    lu::platform::FileDescriptor fileDescriptor(1);

    ASSERT_EQ(fileDescriptor, nullptr);
    int fileID = fileDescriptor;
    ASSERT_TRUE(fileID == 1);
    lu::platform::FileDescriptor fileDescriptorSameComp(1);
    ASSERT_TRUE(fileDescriptor == fileDescriptorSameComp);
    lu::platform::FileDescriptor fileDescriptorDiffComp(2);
    ASSERT_TRUE(fileDescriptor != fileDescriptorDiffComp);
}