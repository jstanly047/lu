#include <storage/db/DBManager.h>

using namespace lu::storage::db;

DBManager::DBManager(const soci::backend_factory& factory, const std::string& connectionStr, unsigned int bulkWriteThreshold):
    m_sociSession(factory, connectionStr),
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

void DBManager::execute(const std::string& sql)
{
    m_sociSession << sql;
}
        
