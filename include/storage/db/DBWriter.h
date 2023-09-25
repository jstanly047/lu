#pragma once
#include <soci/soci.h>
#include <glog/logging.h>

namespace lu::storage::db
{
    class DBWriter
    {
    public:
        DBWriter(soci::session& session, unsigned int bulkWriteThreshold);

        template<typename T>
        bool store(const T& object)
        {
            try
            {
                getDBReflection(object).writeToDB(m_sociSession, object);
            }
            catch (std::exception const & e)
            {
                LOG(ERROR) << "DBWriter : " << e.what();
                return false;
            }

            m_writeCount++;

            if (m_writeCount == m_bulkWriteThreshold)
            {
                m_sociTransaction.commit();
                m_writeCount = 0u;
            }

            return true;
        }

        void rollback();

    private:
        soci::session& m_sociSession;
        soci::transaction m_sociTransaction;
        unsigned int m_bulkWriteThreshold;
        unsigned int m_writeCount;
    };
}