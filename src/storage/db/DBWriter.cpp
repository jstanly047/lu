#include <storage/db/DBWriter.h>

using namespace lu::storage::db;

DBWriter::DBWriter(soci::session& session, unsigned int bulkWriteThreshold) :
    m_sociSession(session),
    m_sociTransaction(m_sociSession),
    m_bulkWriteThreshold(bulkWriteThreshold),
    m_writeCount(0U)
{
}
