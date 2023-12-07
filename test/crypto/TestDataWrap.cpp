#include <crypto/DataWrap.h>
#include <cstring>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace lu::crypto;


TEST(TestDataWrap, checkData)
{
    std::string temp="TestStr";
    DataWrap dataWrap(10);
    std::memcpy(dataWrap.getData(), temp.c_str(), temp.size());
    EXPECT_EQ(std::strncmp(reinterpret_cast<const char*>(dataWrap.getData()), temp.c_str(), temp.size() + 1), 0);
}

TEST(TestDataWrap, checkMoveConstruct)
{
    std::string temp="TestStr";
    DataWrap dataWrap(10);
    std::memcpy(dataWrap.getData(), temp.c_str(), temp.size());
    DataWrap moveConst(std::move(dataWrap));
    EXPECT_EQ(std::strncmp(reinterpret_cast<const char*>(moveConst.getData()), temp.c_str(), temp.size() + 1), 0);
    EXPECT_EQ(dataWrap.getData(), nullptr);
}

TEST(TestDataWrap, checkMoveOperator)
{
    std::string temp1="TestStr1";
    std::string temp2="TestStr2";
    DataWrap dataWrap1(10);
    std::memcpy(dataWrap1.getData(), temp1.c_str(), temp1.size());
    DataWrap dataWrap2(12);
    std::memcpy(dataWrap2.getData(), temp2.c_str(), temp2.size());
    EXPECT_EQ(std::strncmp(reinterpret_cast<const char*>(dataWrap1.getData()), temp1.c_str(), temp1.size() + 1), 0);
    EXPECT_EQ(std::strncmp(reinterpret_cast<const char*>(dataWrap2.getData()), temp2.c_str(), temp2.size() + 1), 0);
    EXPECT_EQ(dataWrap1.getLength(), 10u);
    EXPECT_EQ(dataWrap2.getLength(), 12u);
    dataWrap2 = std::move(dataWrap1);
    EXPECT_EQ(std::strncmp(reinterpret_cast<const char*>(dataWrap2.getData()), temp1.c_str(), temp1.size() + 1), 0);
    EXPECT_EQ(std::strncmp(reinterpret_cast<const char*>(dataWrap1.getData()), temp2.c_str(), temp2.size() + 1), 0);
    EXPECT_EQ(dataWrap1.getLength(), 12u);
    EXPECT_EQ(dataWrap2.getLength(), 10u);
}