#pragma once

#include <queue>
#include <pqxx/pqxx>
#include <thread>
#include <mutex>
#include <string>
#include <chrono>
#include <memory>

#include "EmailsInstance.h"

namespace ISXMailDB
{

class PgEmailsWriter
{
public:
    PgEmailsWriter(const std::string& connection_string, 
                   const uint16_t max_queue_size, const std::chrono::milliseconds& thread_sleep_interval);

    ~PgEmailsWriter();

    PgEmailsWriter(const PgEmailsWriter&) = delete;
    PgEmailsWriter& operator=(const PgEmailsWriter&) = delete;

    void AddEmails(EmailsInstance&& emails);


private:
    void ProcessQueue();

    void InsertEmail(const EmailsInstance& emails, pqxx::work& work);
    uint32_t RetriveUserId(const std::string_view user_name, pqxx::transaction_base& ntx);
    uint32_t InsertEmailBody(const std::string_view content, pqxx::transaction_base& transaction);
    void PerformEmailInsertion(const uint32_t sender_id, const uint32_t receiver_id,
                               const std::string_view subject, const uint32_t body_id, 
                               pqxx::transaction_base& transaction);

    std::queue<EmailsInstance> m_queue;
    mutable std::mutex m_mtx;
    std::thread m_worker_thread;
    bool m_stop_thread;
    std::unique_ptr<pqxx::connection> m_conn;
    
    uint16_t m_max_queue_size;
    std::chrono::milliseconds m_thread_sleep_interval;

};

}