#include <reflection/Reflection.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace lu::reflection;

struct TestReflection1
{
    char c;
    int i;
    long long int lli;
    float f;
    double d;
    std::string str;
};

struct TestReflection2
{
    int i;
};

TEST(TestReflection, checkCREAT_LIST)
{
     auto array = {CREAT_LIST(aaa, bbb, ccc)};
     ASSERT_THAT(array, ::testing::ElementsAre("aaa", "bbb", "ccc"));
}

TEST(TestReflection, checkCONCAT_REC)
{
     std::string str = CONCAT_REC("INSERT INTO " , CONV_TO_STRING(MyClass) , "(" , CREAT_STRING_LIST(",", "", "", a,b), ")") ;
     EXPECT_EQ(str, "INSERT INTO MyClass(a,b)");
}

TEST(TestReflection, checkMemberCount)
{
    EXPECT_EQ(number_of_members<TestReflection1>(), 6);
    EXPECT_EQ(number_of_members<TestReflection2>(), 1);
}