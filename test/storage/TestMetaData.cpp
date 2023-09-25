#include <storage/MetaData.h>
#include <soci/mysql/soci-mysql.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace lu::storage;

constexpr char const* const connectString = "db=lu_test user=stanly password='zoo9Ieho!1qaz' host=127.0.0.1 port=3306";

struct GTestClass
{
    std::string name;
    int age;
    double salary;
};
REFLECTION_ALIAS(GTestClass, gtest_class, name, age, salary);

struct MyClass
{
    std::string name;
    int age;
};
REFLECTION(MyClass, name, age);

class TestMetaData : public ::testing::Test
{
public:
    TestMetaData():m_sql(soci::mysql, connectString){}

protected:
    void SetUp() override 
    {
        m_sql << "TRUNCATE TABLE gtest_class";
        m_sql << "TRUNCATE TABLE MyClass";
        m_sql.commit();
    }

    void TearDown() override 
    {
        
    }

    soci::session m_sql;
};

TEST_F(TestMetaData, checkReflectionAliasDB)
{
    GTestClass obj = {"test1", 25, 500.0};
    lu_reflect_members(obj).writeToDB(m_sql, obj);
    auto rows = lu_reflect_members(obj).getFromDB(m_sql);
    ASSERT_EQ(rows.begin()->name, obj.name);
}

TEST_F(TestMetaData, checkReflectionDB)
{
    MyClass obj = {"test2", 25};
    lu_reflect_members(obj).writeToDB(m_sql, obj);
    auto rows = lu_reflect_members(obj).getFromDB(m_sql);
    ASSERT_EQ(rows.begin()->name, obj.name);
}