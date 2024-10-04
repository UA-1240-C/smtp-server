#include "PgEmailsWriter.h"
#include "MailException.h"

#include <iostream>

namespace ISXMailDB
{

PgEmailsWriter::PgEmailsWriter(const std::string_view connection_string, uint32_t host_id,
                               const uint16_t max_queue_size = 100, 
                               const std::chrono::milliseconds& thread_sleep_interval = std::chrono::milliseconds(2000)) 
    : m_max_queue_size(max_queue_size), m_tread_sleep_interval(thread_sleep_interval), 
      m_stop_thread(false) , HOST_ID(host_id)
{
    m_conn =  std::make_unique<pqxx::connection>(std::string(connection_string));
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
    m_queue.push(emails);
}

void PgEmailsWriter::ProcessQueue()
{
    bool should_commit_transaction = true;
    while (true) 
    {   
        std::this_thread::sleep_for(m_tread_sleep_interval);

        if (m_stop_thread)
            break;

        std::unique_lock<std::mutex> lock(m_mtx);
        if (m_queue.empty())
            continue;

        pqxx::work txn(*m_conn);
        while(!m_queue.empty())
        {
            EmailsInstance emails = m_queue.front();
            m_queue.pop();
            if(!InsertEmail(emails, txn))
            {
                should_commit_transaction = false;
            }
        }
        should_commit_transaction ? txn.commit() : txn.abort();     
        should_commit_transaction = true;
    }
}

bool PgEmailsWriter::InsertEmail(const EmailsInstance &emails, pqxx::work &work)
{
    uint32_t sender_id = emails.sender.sender_id, body_id;
    std::vector<uint32_t> receivers_id, email_id;

    try
    {
        for (size_t i = 0; i < emails.receivers.size(); i++) 
        {
            receivers_id.emplace_back(RetriveUserId(emails.receivers[i], work));
        }
        body_id = InsertEmailBody(emails.body, work);

        for (size_t i = 0; i < receivers_id.size(); i++) {
            email_id.emplace_back(PerformEmailInsertion(sender_id, receivers_id[i], emails.subject, body_id, work));
        }

        for(auto&& att: emails.attachments)
        {
            InsertAttachment(att, email_id, work);
        }
        
        return true;
    }
    catch (const std::exception& e)
    {
        std::cout << e.what();
        return false;
    }
}

uint32_t PgEmailsWriter::RetriveUserId(const std::string_view user_name, pqxx::transaction_base &ntx)
{
    try
    {
        return ntx.query_value<uint32_t>("SELECT user_id FROM users WHERE user_name = " + ntx.quote(user_name) 
                                         + "  AND host_id = " + ntx.quote(HOST_ID));
    }
    catch (pqxx::unexpected_rows &e)
    {
        throw MailException("User doesn't exist");
    };
    return 0;
}

uint32_t PgEmailsWriter::InsertEmailBody(const std::string_view content, pqxx::transaction_base &transaction)
{
    pqxx::result body_id;
    try {
        body_id = transaction.exec_params("SELECT * FROM \"mailBodies\" WHERE body_content = $1 LIMIT 1", content);

        if(body_id.empty())
        {
            body_id.clear();
            body_id = transaction.exec_params(
                "INSERT INTO \"mailBodies\" (body_content)VALUES($1) RETURNING mail_body_id"
                , content
            );
        }
    }
    catch (const std::exception& e) {
        throw MailException(e.what());
    }

    return body_id[0].at("mail_body_id").as<uint32_t>();
}

void PgEmailsWriter::InsertAttachment(const std::string_view attachment_data, const std::vector<uint32_t>& email_ids, pqxx::transaction_base &transaction) const
{
    try {
        uint32_t attachment_id = InsertAttachmentData(attachment_data, transaction);

        for(auto&& id: email_ids)
        {
            transaction.exec_params("INSERT INTO \"mailAttachments\" (email_message_id, data_id)VALUES($1, $2)"
                                    , id, attachment_id);
        }
    }
    catch (const std::exception& e) {
        throw MailException(e.what());
    }

}

uint32_t PgEmailsWriter::InsertAttachmentData(const std::string_view attachment_data, pqxx::transaction_base &transaction) const
{
    pqxx::result attachment_id;
    try {
        attachment_id = transaction.exec_params("SELECT * FROM \"attachmentData\" WHERE data_text = $1 LIMIT 1", attachment_data);

        if(attachment_id.empty())
        {
            attachment_id.clear();
            attachment_id = transaction.exec_params(
                "INSERT INTO \"attachmentData\" (data_text)VALUES($1) RETURNING data_id"
                , attachment_data
            );
        }
    }
    catch (const std::exception& e) {
        throw MailException(e.what());
    }

    return attachment_id[0].at("data_id").as<uint32_t>();
}

uint32_t PgEmailsWriter::PerformEmailInsertion(const uint32_t sender_id, const uint32_t receiver_id,
                                const std::string_view subject, const uint32_t body_id, pqxx::transaction_base& transaction)
{
    pqxx::result email_result;
    try {
        email_result = transaction.exec_params(
                "INSERT INTO \"emailMessages\" (sender_id, recipient_id, subject, mail_body_id, is_received) "
                "VALUES ($1, $2, $3, $4, false) RETURNING email_message_id"
                , sender_id, receiver_id,
                subject, body_id
        );
    }
    catch (const std::exception& e) {
        throw MailException(e.what());
    }

    return email_result[0].at("email_message_id").as<uint32_t>();
}
}
