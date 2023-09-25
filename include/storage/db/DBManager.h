#pragma once
#include <storage/db/DBReader.h>
#include <storage/db/DBWriter.h>

namespace lu::storage::db
{
    class DBManager
    {
    public:
        DBManager(const std::string& connectionStr, unsigned int bulkWriteThreshold);
        DBReader& getDBReader();
        DBWriter& getDBWriter();

        template<typename T>
        void truncate()
        {
            static T t;
            getDBReflection(t).truncate(m_sociSession);
        }

        template<typename T>
        soci::rowset<T> load()
        {
            static T t;
            return m_dbReader.load<T>();
        }

        template<typename T>
        bool store(const T& t)
        {
            return m_dbWriter.store(t);
        }

        void commit();

    private:
        soci::session m_sociSession;
        DBReader m_dbReader;
        DBWriter m_dbWriter;
    };
}