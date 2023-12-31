#include <common/FixedString.h>
#include <gtest/gtest.h>
#include <tuple>
#include <unordered_map>

using namespace lu::common;

TEST(TestFixedString, emptyFixedString)
{
    FixedString<10> fixedString;
    EXPECT_TRUE(fixedString == "");
    EXPECT_TRUE(fixedString.empty());
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
    EXPECT_FALSE(fixedString.empty());
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
    EXPECT_FALSE(fixedString.empty());
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

TEST(TestFixedString, TestHasFunction)
{
    FixedString<10> fixedString1("TestStr");
    FixedString<10> fixedString2("TestStr1");
    FixedString<10> fixedString3("TestStr");
    EXPECT_NE(fixedString1.hash(), fixedString2.hash());
    EXPECT_EQ(fixedString1.hash(), fixedString3.hash());
}

TEST(TestFixedString, storeInUnorderdMap)
{
    FixedString<10> fixedString1("TestStr1");
    FixedString<10> fixedString2("TestStr2");
    FixedString<10> fixedString3("TestStr1");
    std::unordered_map<FixedString<10>, int> tempMap;
    tempMap.emplace(fixedString1, 10);
    tempMap.emplace(fixedString2, 20);
    EXPECT_EQ(tempMap.find("TestStr1")->second, 10);
    EXPECT_EQ(tempMap.find("TestStr2")->second, 20);
    EXPECT_EQ(tempMap.emplace(fixedString3, 30).second, false);
}

TEST(TestFixedString, outputStreamOperator)
{
    FixedString<10> myString("Hello");
    std::ostringstream oss;
    oss << myString;
    EXPECT_EQ(oss.str(), "Hello");

    FixedString<10> emptyString;
    oss.str("");
    oss << emptyString;
    EXPECT_EQ(oss.str(), "");
}