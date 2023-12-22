#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <storage/db/DBTestStructs.h>
#include <storage/db/DBManager.h>

#include <soci/mysql/soci-mysql.h>
#include <soci/sqlite3/soci-sqlite3.h>

using namespace lu::storage::db;

namespace
{
     const std::string create_GTestClass = R"( 
        CREATE TABLE gtest_class  (
            name TEXT,
            age INTEGER,
            salary DOUBLE
        )
    )";

    const std::string create_MyClass = R"(
        CREATE TABLE MyClass  (
            name TEXT,
            age INTEGER
        )
    )";

    const std::string createFixedStr = R"(
        CREATE TABLE FixedStr  (
            name TEXT,
            age INTEGER,
            description TEXT
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
    }

    void TearDown() override 
    {
        m_dbManger.commit();
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
    //m_dbManger.truncate<GTestClass>();
}

TEST_F(TestMetaData, checkReflectionDB)
{
    m_dbManger.execute(create_MyClass);
    MyClass obj = {"test2", 25};
    m_dbManger.store(obj);
    auto rows = m_dbManger.load<MyClass>();
    ASSERT_EQ(rows.begin()->name, obj.name);
    //m_dbManger.truncate<MyClass>();
}

TEST_F(TestMetaData, DBReaderAndWriter)
{
    m_dbManger.execute(create_MyClass);
    MyClass obj = {"test2", 25};
    m_dbManger.getDBWriter().store(obj);
    auto rows = m_dbManger.getDBReader().load<MyClass>();
    ASSERT_EQ(rows.begin()->name, obj.name);
    //m_dbManger.truncate<MyClass>();
}

TEST_F(TestMetaData, checkReflectionWithFixedString)
{
    m_dbManger.execute(createFixedStr);
    FixedStr obj = {"test2", 25, "Test with FixedString"};
    m_dbManger.store(obj);
    auto rows = m_dbManger.load<FixedStr>();
    auto temp = rows.begin();
    ASSERT_EQ(temp->name, obj.name);
    ASSERT_EQ(temp->age, obj.age);
    ASSERT_EQ(temp->description, obj.description);
    //m_dbManger.truncate<MyClass>();
}


TEST_F(TestMetaData, checkDBQueryWithWhereClause)
{
    m_dbManger.execute(createFixedStr);
    FixedStr obj1 = {"filter1", 30, "Test filter1"};
    m_dbManger.store(obj1);
    FixedStr obj2 = {"filter2", 31, "Test filter3"};
    m_dbManger.store(obj2);
    FixedStr obj3 = {"filter2", 31, "Test filter2"};
    m_dbManger.store(obj3);
    auto rows = m_dbManger.load<FixedStr>();
    std::size_t rowCount = 0;
    for ([[maybe_unused]]auto row: rows) 
    {
        rowCount++;
    }
    EXPECT_EQ(rowCount, 3);
    rows = m_dbManger.load<FixedStr>("name='filter1'");
    rowCount = 0;
    for ([[maybe_unused]]auto row: rows) 
    {
        rowCount++;
    }
    EXPECT_EQ(rowCount, 1);
    rows = m_dbManger.load<FixedStr>("age=31");
    rowCount = 0;
    for ([[maybe_unused]]auto row: rows) 
    {
        rowCount++;
    }
    EXPECT_EQ(rowCount, 2);
    //m_dbManger.truncate<MyClass>();
}