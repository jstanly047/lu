#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <storage/db/DBTestStructs.h>
#include <storage/db/DBManager.h>

#include <soci/mysql/soci-mysql.h>
#include <soci/sqlite3/soci-sqlite3.h>

using namespace lu::storage::db;

namespace
{
     constexpr char* create_GTestClass = R"( 
        CREATE TABLE gtest_class  (
            name TEXT,
            age INTEGER,
            salary DOUBLE
        )
    )";

    constexpr char* create_MyClass = R"(
        CREATE TABLE MyClass  (
            name TEXT,
            age INTEGER
        )
    )";
}

class TestMetaData : public ::testing::Test
{
public:
    TestMetaData():
            m_dbManger(soci::sqlite3, ":memory:", 100)
            {}

protected:
    void SetUp() override 
    {
        //m_dbManger.truncate<GTestClass>();
        //m_dbManger.truncate<MyClass>();
        m_dbManger.commit();
    }

    void TearDown() override 
    {
    }

    DBManager m_dbManger;
};

TEST_F(TestMetaData, checkReflectionAliasDB)
{
    m_dbManger.execute(create_GTestClass);
    GTestClass obj = {"test1", 25, 500.0};
    m_dbManger.store(obj);
    auto rows = m_dbManger.load<GTestClass>();
    ASSERT_EQ(rows.begin()->name, obj.name);
}

TEST_F(TestMetaData, checkReflectionDB)
{
    m_dbManger.execute(create_MyClass);
    MyClass obj = {"test2", 25};
    m_dbManger.store(obj);
    auto rows = m_dbManger.load<MyClass>();
    ASSERT_EQ(rows.begin()->name, obj.name);
}