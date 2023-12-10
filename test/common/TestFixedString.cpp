#include <common/FixedString.h>
#include <gtest/gtest.h>
#include <tuple>

using namespace lu::common;

TEST(TestFixedString, emptyFixedString)
{
    FixedString<10> fixedString;
    EXPECT_TRUE(fixedString == "");
    EXPECT_FALSE(fixedString == "A");
    EXPECT_TRUE(std::strncmp(fixedString.getCString(), "", 3) == 0);
    EXPECT_FALSE(std::strncmp(fixedString.getCString(), "A", 3) == 0);
    EXPECT_TRUE(fixedString == std::string(""));
    EXPECT_FALSE(fixedString == std::string("A"));
    FixedString<10> fixedStringSame;
    EXPECT_TRUE(fixedString == fixedStringSame);
    FixedString<10> fixedStringNotEqual("A");
    EXPECT_FALSE(fixedString == fixedStringNotEqual);
}

TEST(TestFixedString, stringConstructor)
{
    FixedString<10> fixedString("TestString");
    EXPECT_TRUE(fixedString == "TestString");
    EXPECT_FALSE(fixedString == "TestStrin");
    EXPECT_TRUE(std::strncmp(fixedString.getCString(), "TestString", 10) == 0);
    EXPECT_FALSE(std::strncmp(fixedString.getCString(), "TestStrings", 11) == 0);
    EXPECT_TRUE(fixedString == std::string("TestString"));
    EXPECT_FALSE(fixedString == std::string("TestStringExtra"));
    FixedString<10> fixedStringSame("TestString");
    EXPECT_TRUE(fixedString == fixedStringSame);
    FixedString<10> fixedStringNotEqual("");
    EXPECT_FALSE(fixedString == fixedStringNotEqual);
}

TEST(TestFixedString, constCharConstructorWithLargerSize)
{
    FixedString<10> fixedString("TestStringExtra");
    EXPECT_TRUE(fixedString == "TestString");
    EXPECT_FALSE(fixedString == "TestStrin");
    EXPECT_TRUE(std::strncmp(fixedString.getCString(), "TestString", 10) == 0);
    EXPECT_FALSE(std::strncmp(fixedString.getCString(), "TestStringExtra", 15) == 0);
    EXPECT_TRUE(fixedString == "TestString");
    EXPECT_FALSE(fixedString == "TestStringExtra");
    FixedString<10> fixedStringSame("TestString");
    EXPECT_TRUE(fixedString == fixedStringSame);
    FixedString<10> fixedStringNotEqual("");
    EXPECT_FALSE(fixedString == fixedStringNotEqual);
}

TEST(TestFixedString, stringConstructorWithLargerSize)
{
    std::string str = "TestStringExtra";
    FixedString<10> fixedString(str);
    EXPECT_TRUE(fixedString == "TestString");
    EXPECT_FALSE(fixedString == "TestStrin");
    EXPECT_TRUE(std::strncmp(fixedString.getCString(), "TestString", 10) == 0);
    EXPECT_FALSE(std::strncmp(fixedString.getCString(), "TestStringExtra", 15) == 0);
    EXPECT_TRUE(fixedString == std::string("TestString"));
    EXPECT_FALSE(fixedString == std::string("TestStringExtra"));
    FixedString<10> fixedStringSame("TestString");
    EXPECT_TRUE(fixedString == fixedStringSame);
    FixedString<10> fixedStringNotEqual("");
    EXPECT_FALSE(fixedString == fixedStringNotEqual);
}

TEST(TestFixedString, charConstruct)
{
    FixedString<10> fixedString('A');
    EXPECT_TRUE(fixedString == "A");
    EXPECT_FALSE(fixedString == "TestStrin");
    EXPECT_TRUE(std::strncmp(fixedString.getCString(), "A", 1) == 0);
    EXPECT_FALSE(std::strncmp(fixedString.getCString(), "TestStringExtra", 15) == 0);
    EXPECT_TRUE(fixedString == std::string("A"));
    EXPECT_FALSE(fixedString == std::string("TestStringExtra"));
    FixedString<10> fixedStringSame("A");
    EXPECT_TRUE(fixedString == fixedStringSame);
    FixedString<10> fixedStringNotEqual("");
    EXPECT_FALSE(fixedString == fixedStringNotEqual);
}
TEST(TestFixedString, implicitStringConversion)
{
    FixedString<10> fixedString("TestStr");
    std::string str = fixedString;
    EXPECT_TRUE(str == "TestStr");
}