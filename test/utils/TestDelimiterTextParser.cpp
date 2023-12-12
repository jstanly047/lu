#include <utils/DelimiterTextParser.h>
#include <string>
#include <gtest/gtest.h>

using namespace lu::utils;

namespace
{
    const char TEXT_FILE_DELIMETER = '~';
}

TEST(TestDelimiterTextParser, emptyString)
{
    std::string empty;
    DelimiterTextParser input(empty, TEXT_FILE_DELIMETER);
    ASSERT_THROW({
        try 
        {
            input.next();
        }
        catch(const std::logic_error& e)
        {
            EXPECT_STREQ("Item [1] does not exist in line [1]", e.what());
            throw;
        }} , std::logic_error);
}

TEST(TestDelimiterTextParser, nullString)
{
    
    std::string inputStr="null~NULL~Test";
    DelimiterTextParser input(inputStr, TEXT_FILE_DELIMETER);

    EXPECT_TRUE(input.next().empty());
    EXPECT_TRUE(input.next().empty());
    EXPECT_EQ(input.next(), "Test");
}

TEST(TestDelimiterTextParser, nonEmptyStringWithoutDelimiter)
{
    std::string line = "Test";
    DelimiterTextParser input(line, TEXT_FILE_DELIMETER);

    EXPECT_EQ(input.next(), "Test");
    ASSERT_THROW({
        try 
        {
            input.next();
        }
        catch(const std::logic_error& e)
        {
            EXPECT_STREQ("Item [2] does not exist in line [1]", e.what());
            throw;
        }} , std::logic_error);
}

TEST(TestDelimiterTextParser, nonEmptyStringWithDelimiter)
{
    std::string line =  "Value1";
    line += TEXT_FILE_DELIMETER;
    line += "Value2";
    line += TEXT_FILE_DELIMETER;
    line += "Value3";
    DelimiterTextParser input(line, TEXT_FILE_DELIMETER);

    EXPECT_EQ(input.next(), "Value1");
    EXPECT_EQ(input.next(), "Value2");
    EXPECT_EQ(input.next(), "Value3");
    ASSERT_THROW({
        try 
        {
            input.next();
        }
        catch(const std::logic_error& e)
        {
            EXPECT_STREQ("Item [4] does not exist in line [1]", e.what());
            throw;
        }} , std::logic_error);
}

TEST(TestDelimiterTextParser, checkNonStrings)
{
    std::string line =  "Value1";
    line += TEXT_FILE_DELIMETER;
    line += TEXT_FILE_DELIMETER;
    line += "1";
    line += TEXT_FILE_DELIMETER;
    line += "1.0";
    line += TEXT_FILE_DELIMETER;
    line += "2.0";
    line += TEXT_FILE_DELIMETER;
    line += "C";
    line += TEXT_FILE_DELIMETER;
    DelimiterTextParser input(line, TEXT_FILE_DELIMETER);
    

    EXPECT_EQ(input.next(), "Value1");
    EXPECT_EQ(input.next(), "");
    EXPECT_EQ(input.nextInt(), 1);
    EXPECT_EQ(input.nextFloat(), 1.0F);
    EXPECT_EQ(input.nextDouble(), 2.0);
    EXPECT_EQ(input.nextChar(), 'C');
    ASSERT_THROW({
        try 
        {
            input.nextChar();
        }
        catch(const std::logic_error& e)
        {
            EXPECT_STREQ("Item [7] does not exist in line [1]", e.what());
            throw;
        }} , std::logic_error);
}

TEST(TestDelimiterTextParser, checkNonStringsWithSpace)
{
    std::string line =  "  Value1 ";
    line += TEXT_FILE_DELIMETER;
    line += TEXT_FILE_DELIMETER;
    line += "1 ";
    line += TEXT_FILE_DELIMETER;
    line += "  1.0";
    line += TEXT_FILE_DELIMETER;
    line += "2.0  ";
    line += TEXT_FILE_DELIMETER;
    line += "    C  ";
    line += TEXT_FILE_DELIMETER;
    DelimiterTextParser input(line, TEXT_FILE_DELIMETER);
    

    EXPECT_EQ(input.next(), "Value1");
    EXPECT_EQ(input.next(), "");
    EXPECT_EQ(input.nextInt(), 1);
    EXPECT_EQ(input.nextFloat(), 1.0F);
    EXPECT_EQ(input.nextDouble(), 2.0);
    EXPECT_EQ(input.nextChar(), 'C');
}

TEST(TestDelimiterTextParser, boolValue)
{
    std::string empty="0~1~~2";
    DelimiterTextParser input(empty, TEXT_FILE_DELIMETER);

    EXPECT_EQ(input.nextBool(), false);
    EXPECT_EQ(input.nextBool(), true);
    EXPECT_EQ(input.nextBool(), false);
    EXPECT_EQ(input.nextBool(), false);
    ASSERT_THROW({
        try 
        {
            input.nextBool();
        }
        catch(const std::logic_error& e)
        {
            EXPECT_STREQ("Item [5] does not exist in line [1]", e.what());
            throw;
        }} , std::logic_error);
}

TEST(TestDelimiterTextParser, dateTimeTextTest)
{
    auto checkTime=[](const std::tm& local, int year, int month, int date, int hour, int min, int sec)
    {
        return (local.tm_year == year - 1900) && (local.tm_mon == month - 1) 
                && (local.tm_mday == date) && (local.tm_hour == hour) 
                && (local.tm_min == min) && (local.tm_sec == sec);
    };

    std::string line =  "20211011010203";
    line += TEXT_FILE_DELIMETER;
    line += "2021-10-11 01:02:03";

    DelimiterTextParser input(line, TEXT_FILE_DELIMETER);
    std::time_t tm= input.nextDateTime();
    EXPECT_TRUE(checkTime(*std::localtime(&tm), 2021, 10, 11, 01, 02, 03));
    tm = input.nextDateTime("%Y-%m-%d %H:%M:%S");
    EXPECT_TRUE(checkTime(*std::localtime(&tm), 2021, 10, 11, 01, 02, 03));
    ASSERT_THROW({
        try 
        {
            input.nextDateTime();
        }
        catch(const std::logic_error& e)
        {
            EXPECT_STREQ("Item [3] does not exist in line [1]", e.what());
            throw;
        }} , std::logic_error);
}

TEST(TestDelimiterTextParser, formatExceptions)
{
    std::string empty="A~B~C~D~E";
    DelimiterTextParser input(empty, TEXT_FILE_DELIMETER);

    ASSERT_THROW(input.nextInt(), std::logic_error);
    ASSERT_THROW(input.nextBool(), std::logic_error);
    ASSERT_THROW(input.nextDouble(), std::logic_error);
    ASSERT_THROW(input.nextFloat(), std::logic_error);
    ASSERT_THROW(input.nextDateTime(), std::logic_error);
}
