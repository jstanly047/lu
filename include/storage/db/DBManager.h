#pragma once
#include <storage/db/DBReader.h>
#include <storage/db/DBWriter.h>

namespace lu::storage::db
{
    class DBManager
    {
    public:
        DBManager(const soci::backend_factory& factory, const std::string& connectionStr, unsigned int bulkWriteThreshold);
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
            return m_dbReader.load<T>();
        }

        template<typename T>
        soci::rowset<T> load(const std::string& whereClause)
        {
            return m_dbReader.load<T>(whereClause);
        }

        template<typename T>
        bool store(const T& t)
        {
            return m_dbWriter.store(t);
        }

        void commit();
        
        void begin();

        void execute(const std::string& sql);

        

    private:
        soci::session m_sociSession;
        DBReader m_dbReader;
        DBWriter m_dbWriter;
    };
}