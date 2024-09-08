#include "PgEmailsWriter.h"
#include "MailException.h"

namespace ISXMailDB
{

PgEmailsWriter::PgEmailsWriter(const std::string& connection_string,
                               const uint16_t max_queue_size = 100, 
                               const std::chrono::milliseconds& thread_sleep_interval = std::chrono::milliseconds(2000)) 
    : m_max_queue_size(max_queue_size), m_thread_sleep_interval(thread_sleep_interval), 
    m_stop_thread(false) 
{
    m_conn =  std::make_unique<pqxx::connection>(connection_string);
    m_worker_thread = std::thread(&PgEmailsWriter::ProcessQueue, this);
}

PgEmailsWriter::~PgEmailsWriter()
{
    m_stop_thread = true;
    m_worker_thread.join();
}

void PgEmailsWriter::AddEmails(EmailsInstance &&emails)
{
    std::lock_guard<std::mutex> lockm(m_mtx);
    if (m_queue.size() >= m_max_queue_size)
    {
        throw MailException("Too many mails in queue");
    }
}

void PgEmailsWriter::ProcessQueue()
{
    while (true) 
    {
        std::this_thread::sleep_for(m_thread_sleep_interval);

        if (m_stop_thread)
            break;

        std::unique_lock<std::mutex> lock(m_mtx);
        if (m_queue.empty())
            continue;

        pqxx::work txn(*m_conn);
        // while(!m_queue.empty())
        // {

        // }
//         // Process queue if not empty
//         std::vector<std::string> dataBatch;
//         while (!queue.empty()) {
//             dataBatch.push_back(queue.front());
//             queue.pop();
//         }

//         lock.unlock();

//         if (!dataBatch.empty()) {
//             writeToDatabase(dataBatch);  // Write all collected data in one batch
//         }
// }
    }
}

void PgEmailsWriter::InsertEmail(const EmailsInstance &emails, pqxx::work &work)
{
    uint32_t sender_id = emails.sender.sender_id, body_id;
    std::vector<uint32_t> receivers_id;

    try
    {
        for (size_t i = 0; i < emails.receivers.size(); i++) 
        {
            receivers_id.emplace_back(RetriveUserId(emails.receivers[i], work));
        }
        body_id = InsertEmailBody(emails.body, work);

        for (size_t i = 0; i < receivers_id.size(); i++) {
            PerformEmailInsertion(sender_id, receivers_id[i], emails.subject, body_id, work);
        }
    }
    catch (const std::exception& e)
    {
        throw MailException(e.what());
    }
}

uint32_t PgEmailsWriter::RetriveUserId(const std::string_view user_name, pqxx::transaction_base &ntx)
{
    // try
    // {
    //     return ntx.query_value<uint32_t>("SELECT user_id FROM users WHERE user_name = " + ntx.quote(user_name) 
    //                                      + "  AND host_id = " + ntx.quote(m_host_id));
    // }
    // catch (pqxx::unexpected_rows &e)
    // {
    //     throw MailException("User doesn't exist");
    // };
    return 0;
}

uint32_t PgEmailsWriter::InsertEmailBody(const std::string_view content, pqxx::transaction_base &transaction)
{
    return 0;
}

void PgEmailsWriter::PerformEmailInsertion(const uint32_t sender_id, const uint32_t receiver_id, const std::string_view subject, const uint32_t body_id, pqxx::transaction_base &transaction)
{

}
}
