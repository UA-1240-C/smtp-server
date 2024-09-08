#pragma once

#include <pqxx/pqxx>

#include "ConnectionPool.h"
#include "PgEmailsWriter.h"

namespace ISXMailDB
{

class PgManager
{
public:
    PgManager(const std::string_view connection_string, const std::string_view host_name, bool should_cache_emails);

    ConnectionPool<pqxx::connection>& get_connection_pool() { return *m_connection_pool; }
    std::shared_ptr<PgEmailsWriter> get_emails_writer () {return m_emails_writer; }

    std::string get_host_name() const { return HOST_NAME; }
    uint32_t get_host_id() const {return m_host_id; }

private:
    void Init();

    void InitEmaisWriter();
    void InitConnectionPool();
    void InsertHost();

    bool m_should_cache_emails;
    std::shared_ptr<PgEmailsWriter> m_emails_writer;
    const uint32_t MAX_WRITER_QUEUE_SIZE = 100;
    const std::chrono::milliseconds WRITER_TIMEOUT{2000};

    const std::string CONNECTION_STRING;
    std::unique_ptr<ConnectionPool<pqxx::connection>> m_connection_pool;
    const uint16_t POOL_INITIAL_SIZE = 10;
    
    const std::string HOST_NAME;
    uint32_t m_host_id;
};

}