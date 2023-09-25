#include <storage/db/DBManager.h>
#include <soci/mysql/soci-mysql.h>

using namespace lu::storage::db;

DBManager::DBManager(const std::string& connectionStr, unsigned int bulkWriteThreshold):
    m_sociSession(soci::mysql, connectionStr),
    m_dbReader(m_sociSession),
    m_dbWriter(m_sociSession, bulkWriteThreshold)
{

}

DBReader& DBManager::getDBReader()
{
    return m_dbReader;
}

DBWriter& DBManager::getDBWriter()
{
    return m_dbWriter;
}

void DBManager::commit()
{
    m_sociSession.commit();
}
        
