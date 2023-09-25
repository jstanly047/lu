#pragma once
#include <soci/soci.h>

namespace lu::storage::db
{
    class DBReader
    {
    public:
        DBReader(soci::session& session): m_sociSession(session){}

        template<typename T>
        soci::rowset<T> load()
        {
            static T t;
            return getDBReflection(t).getFromDB(m_sociSession);
        }
        
    private:
        soci::session& m_sociSession;
    };
}