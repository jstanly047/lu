#include <common/Config.h>
#include <limits>
#include <gtest/gtest.h>

using namespace lu::common;

TEST(TestConfig, loadFromFileFail)
{
    ASSERT_FALSE(Configurations::loadFromFile("resource/Configuration"));
}

TEST(TestConfig, loadFromFile)
{
    EXPECT_EQ(Configurations::getValue<bool>("G1", "CHECK_BOOL"), false);
    EXPECT_EQ(Configurations::getValue<char>("G1", "CHECK_CHAR"), '\0');
    EXPECT_EQ(Configurations::getValue<int>("G1", "CHECK_INT"), 0);
    EXPECT_EQ(Configurations::getValue<unsigned int>("G1", "CHECK_UINT"), 0U);
    EXPECT_EQ(Configurations::getValue<long long>("G2", "CHECK_LONG_LONG"), 0LL);
    EXPECT_EQ(Configurations::getValue<unsigned long long>("G2", "CHECK_U_LONG_LONG"),0ULL );
    EXPECT_EQ(Configurations::getValue<float>("G2", "CHECK_FLOAT"), 0.0);
    EXPECT_EQ(Configurations::getValue<double>("G2", "CHECK_DOUBLE"), 0.0);
    EXPECT_EQ(Configurations::getValue<std::string>("G2", "CHECK_STRING"), "");

    ASSERT_TRUE(Configurations::loadFromFile("resource/Configuration.txt"));
    EXPECT_EQ(Configurations::getValue<bool>("G1", "CHECK_BOOL"), true);
    EXPECT_EQ(Configurations::getValue<char>("G1", "CHECK_CHAR"), 'C');
    EXPECT_EQ(Configurations::getValue<int>("G1", "CHECK_INT"), std::numeric_limits<int>::max());
    EXPECT_EQ(Configurations::getValue<unsigned int>("G1", "CHECK_UINT"), std::numeric_limits<unsigned int>::max());
    EXPECT_EQ(Configurations::getValue<long long>("G2", "CHECK_LONG_LONG"), std::numeric_limits<long long>::max());
    EXPECT_EQ(Configurations::getValue<unsigned long long>("G2", "CHECK_U_LONG_LONG"), std::numeric_limits<unsigned long long>::max());
    EXPECT_EQ(Configurations::getValue<float>("G2", "CHECK_FLOAT"), 0.111f);
    EXPECT_EQ(Configurations::getValue<double>("G2", "CHECK_DOUBLE"), 0.2222);
    EXPECT_EQ(Configurations::getValue<std::string>("G2", "CHECK_STRING"), "OMS Thread");
}

TEST(TestConfig, getFromFile)
{
    auto configurations = Configurations::getFromFile("resource", "Configuration.txt");
    EXPECT_EQ(configurations.get<bool>("G1", "CHECK_BOOL"), true);
    EXPECT_EQ(configurations.get<char>("G1", "CHECK_CHAR"), 'C');
    EXPECT_EQ(configurations.get<int>("G1", "CHECK_INT"), std::numeric_limits<int>::max());
    EXPECT_EQ(configurations.get<unsigned int>("G1", "CHECK_UINT"), std::numeric_limits<unsigned int>::max());
    EXPECT_EQ(configurations.get<long long>("G2", "CHECK_LONG_LONG"), std::numeric_limits<long long>::max());
    EXPECT_EQ(configurations.get<unsigned long long>("G2", "CHECK_U_LONG_LONG"), std::numeric_limits<unsigned long long>::max());
    EXPECT_EQ(configurations.get<float>("G2", "CHECK_FLOAT"), 0.111f);
    EXPECT_EQ(configurations.get<double>("G2", "CHECK_DOUBLE"), 0.2222);
    EXPECT_EQ(configurations.get<std::string>("G2", "CHECK_STRING"), "OMS Thread");
}