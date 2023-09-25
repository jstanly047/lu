#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <storage/db/DBTestStructs.h>
#include <storage/db/DBManager.h>

using namespace lu::storage::db;

class TestMetaData : public ::testing::Test
{
public:
    TestMetaData():
            m_dbManger(connectString, 100)
            {}

protected:
    void SetUp() override 
    {
        m_dbManger.truncate<GTestClass>();
        m_dbManger.truncate<MyClass>();
        m_dbManger.commit();
    }

    void TearDown() override 
    {
    }

    DBManager m_dbManger;
};

TEST_F(TestMetaData, checkReflectionAliasDB)
{
    GTestClass obj = {"test1", 25, 500.0};
    m_dbManger.store(obj);
    auto rows = m_dbManger.load<GTestClass>();
    ASSERT_EQ(rows.begin()->name, obj.name);
}

TEST_F(TestMetaData, checkReflectionDB)
{
    MyClass obj = {"test2", 25};
    m_dbManger.store(obj);
    auto rows = m_dbManger.load<MyClass>();
    ASSERT_EQ(rows.begin()->name, obj.name);
}