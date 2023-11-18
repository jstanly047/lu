#include <crypto/Hash.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace lu::crypto;

namespace
{
    const std::string data = "LU_PLATFORM_TEST";
}

TEST(TestHash, sha)
{
    Hash<HashAlgo::SHA> shaHash;
    EXPECT_EQ(shaHash.init(), true);
    EXPECT_EQ(shaHash.getBase64Hash(data), "A1mAgt0BH+Xvx1/TG8dkt/wDgLg=");
    EXPECT_NE(shaHash.getBase64Hash(data), "A1mAgt0BH+Xvx1/TG8dkt/wDgLg=1");
}

TEST(TestHash, checkCleanup)
{
    Hash<HashAlgo::SHA> shaHash;
    EXPECT_EQ(shaHash.init(), true);
    EXPECT_EQ(shaHash.getBase64Hash(data), "A1mAgt0BH+Xvx1/TG8dkt/wDgLg=");
    EXPECT_EQ(shaHash.getBase64Hash(data), "A1mAgt0BH+Xvx1/TG8dkt/wDgLg=");
}

TEST(TestHash, sha224)
{
    Hash<HashAlgo::SHA224> shaHash;
    EXPECT_EQ(shaHash.init(), true);
    EXPECT_EQ(shaHash.getBase64Hash(data), "9180gz6YnY1Lqg85r4KNSLBu9WqU8mk2Xn+DLA==");
    EXPECT_NE(shaHash.getBase64Hash(data), "ss9180gz6YnY1Lqg85r4KNSLBu9WqU8mk2Xn+DLA==");
}

TEST(TestHash, sha256)
{
    Hash<HashAlgo::SHA256> shaHash;
    EXPECT_EQ(shaHash.init(), true);
    EXPECT_EQ(shaHash.getBase64Hash(data), "jaCv8U5MNBIlfsEGD4AK4SOMuK983x1sjTpql9UFKSM=");
    EXPECT_NE(shaHash.getBase64Hash(data), "jaCv8U5MNBIlfsEGD4AK4SOMlK983x1sjTpql9UFKSM=");
}

TEST(TestHash, sha384)
{
    Hash<HashAlgo::SHA384> shaHash;
    EXPECT_EQ(shaHash.init(), true);
    EXPECT_EQ(shaHash.getBase64Hash(data), "xp+DBbK7Vof1u6iC8cBkMoTxsqqBXIDCny/uq+YOIWxeRGEyGq4CJBqVaarjwAhd");
    EXPECT_NE(shaHash.getBase64Hash(data), "sdf=");
}

TEST(TestHash, sha512)
{
    Hash<HashAlgo::SHA512> shaHash;
    EXPECT_EQ(shaHash.init(), true);
    EXPECT_EQ(shaHash.getBase64Hash(data), "F9vu+7yFR5hIT8LlKjhRzZEGN+ZbQ/KsWb2tEUdIbGOvJyYkMVrqUjrcEwbGFrSsUtbXwaLaS9vIQc5gDv2XbA==");
    EXPECT_NE(shaHash.getBase64Hash(data), "sdfsdf=");
}

TEST(TestHash, md5)
{
    Hash<HashAlgo::MD5> mdHash;
    EXPECT_EQ(mdHash.init(), true);
    EXPECT_EQ(mdHash.getBase64Hash(data), "qi3tc6ofkB93coEvYMkcjg==");
    EXPECT_NE(mdHash.getBase64Hash(data), "sdfsdf=");
}