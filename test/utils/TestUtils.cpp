#include <utils/Utils.h>
#include <string>
#include <gtest/gtest.h>
#include <gmock/gmock.h>



class TestUtils : public ::testing::Test
{
};


TEST_F(TestUtils, stringToEpocUTC)
{
    ASSERT_EQ(lu::utils::Utils::getUTCDateTime("19700101000000"), 0);
    ASSERT_EQ(lu::utils::Utils::getUTCDateTime("1970/01/01 00:00:00", "%Y/%m/%d %H:%M:%S"), 0);
}

TEST_F(TestUtils, stringToEpocLocal)
{
    ASSERT_EQ(lu::utils::Utils::getDateTime("19700101053000"), 0);
    ASSERT_EQ(lu::utils::Utils::getDateTime("1970/01/01 05:30:00", "%Y/%m/%d %H:%M:%S"), 0);
}

/*
TEST_F(TestUtils, epocToStr)
{
    ASSERT_EQ(lu::utils::Utils::getDateTimeStr(0), "19700101073000");
    ASSERT_EQ(lu::utils::Utils::getDateTimeStr(0,"%Y/%m/%d %H:%M:%S"), "1970/01/01 07:30:00");
}*/

TEST_F(TestUtils, charSplit)
{
    auto retVal = lu::utils::Utils::splitString<char, char>("A,B,C", ',');
    ASSERT_THAT(retVal, ::testing::ElementsAre('A','B','C'));
}

TEST_F(TestUtils, intSplit)
{
    auto retVal = lu::utils::Utils::splitString<int, char>("1,2,3", ',');
    ASSERT_THAT(retVal, ::testing::ElementsAre(1,2,3));
}

TEST_F(TestUtils, stringSplit)
{
    auto retVal = lu::utils::Utils::splitString<std::string, char>("AB,CD,EF", ',');
    ASSERT_THAT(retVal, ::testing::ElementsAre("AB", "CD", "EF"));
}

TEST_F(TestUtils, stringSplitBySplit)
{
    auto retVal = lu::utils::Utils::splitString<std::string, std::string>("AB==CD==EF", "==");
    ASSERT_THAT(retVal, ::testing::ElementsAre("AB", "CD", "EF"));
}

TEST_F(TestUtils, testTimeSizeIs64Bit)
{
    ASSERT_EQ(sizeof(std::time_t), 8U);
}