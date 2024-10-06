#include <memory>

#include "MailException.h"
#include "PgMailDB.h"

namespace ISXMailDB
{
PgMailDB::PgMailDB(std::string_view host_name)
    : IMailDB(host_name)
{
}

PgMailDB::PgMailDB(const PgMailDB& other) : IMailDB(other.m_host_name)
{
}

PgMailDB::~PgMailDB()
{   
    Disconnect();
}


void PgMailDB::Connect(const std::string& connection_string)
{
    try {
        m_conn = std::make_unique<pqxx::connection>(connection_string);
        InsertHost(m_host_name);
    }
    catch (const std::exception &e) {
        throw MailException(e.what());
    }
}

void PgMailDB::Disconnect()
{
    if (IsConnected())
    {
        m_conn->close();
    }
}

bool PgMailDB::IsConnected() const
{
    if (m_conn) 
    {
        return m_conn->is_open();
    }

    return false;
}

void PgMailDB::SignUp(const std::string_view user_name, const std::string_view password)
{
    pqxx::work tx(*m_conn);
    try
    {
        tx.exec_params0("SELECT 1 FROM users WHERE host_id = $1 AND user_name = $2", m_host_id, user_name);

        std::string hashed_password = HashPassword(std::string(password));

        tx.exec_params(
            "INSERT INTO users (host_id, user_name, password_hash)"
            "VALUES ($1, $2, $3)",
            m_host_id, user_name, hashed_password);
    }
    catch (const pqxx::unexpected_rows& e)
    {
        throw MailException("User already exists");
    }
    catch (const MailException& e)
    {
        throw;
    }
    tx.commit();
}

void PgMailDB::InsertHost(const std::string_view host_name)
{
    pqxx::work tx(*m_conn);
    try
    {
        m_host_id = tx.query_value<uint32_t>("SELECT host_id FROM hosts WHERE host_name = " + tx.quote(m_host_name));
    }
    catch (pqxx::unexpected_rows& e)
    {
        pqxx::result host_id_result = tx.exec_params(
            "INSERT INTO hosts (host_name) VALUES ( $1 ) RETURNING host_id", m_host_name);

        m_host_id = host_id_result[0][0].as<uint32_t>();
    }
    tx.commit();
}

std::string PgMailDB::HashPassword(const std::string& password)
{
    std::string hashed_password(crypto_pwhash_STRBYTES, '\0');

    if (crypto_pwhash_str(hashed_password.data(),
        password.c_str(),
        password.size(),
        crypto_pwhash_OPSLIMIT_INTERACTIVE,
        crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0)
    {
        throw MailException("Password hashing failed");
    }

    return hashed_password;
}

bool PgMailDB::VerifyPassword(const std::string& password, const std::string& hashed_password)
{
    return crypto_pwhash_str_verify(hashed_password.c_str(), password.c_str(),
                                    password.size()) == 0;
}

void PgMailDB::Login(const std::string_view user_name, const std::string_view password)
{
    pqxx::nontransaction ntx(*m_conn);

    try
    {
        std::string hashed_password = ntx.query_value<std::string>("SELECT password_hash FROM users "
            "WHERE user_name = " + ntx.quote(user_name) 
            +  " AND host_id = " + ntx.quote(m_host_id)
            );
        
        if (!VerifyPassword(std::string(password), hashed_password))
        {
            throw MailException("Invalid username or password");
        }

    }
    catch (const pqxx::unexpected_rows &e)
    {
        throw MailException("User with mentioned username doesn't exist");
    }
}

std::vector<User> PgMailDB::RetrieveUserInfo(const std::string_view user_name)
    {
        if (!IsConnected())
        {
            throw MailException("Connection with database lost or was manually already closed");
        }

        pqxx::nontransaction nontransaction(*m_conn);
        pqxx::result user_query_result;
    
        if (user_name.empty())
        {
            user_query_result = nontransaction.exec_params(
                "SELECT u.user_name, u.password_hash, h.host_name FROM users u "
                "LEFT JOIN hosts h ON u.host_id = h.host_id"
            );
        }
        else
        {
            user_query_result = nontransaction.exec_params(
                "SELECT u.user_name, u.password_hash, h.host_name FROM users u "
                "LEFT JOIN hosts h ON u.host_id = h.host_id "
                "WHERE u.user_name = $1"
                , nontransaction.esc(user_name)
            );
        }
        
        std::vector<User> info;
        if (!user_query_result.empty())
        {
            for (auto&& row : user_query_result)
            {
                info.emplace_back(User(row.at("user_name").as<std::string>()
                                  , row.at("password_hash").as<std::string>()
                                  , row.at("host_name").as<std::string>()));
            }

            return info;
        }

        return std::vector<User>();
    }

std::vector<std::string> PgMailDB::RetrieveEmailContentInfo(const std::string_view content)
{
    if (!IsConnected())
    {
        throw MailException("Connection with database lost or was manually already closed");
    }

    pqxx::nontransaction nontransaction(*m_conn);
    pqxx::result content_query_result;
   
    if (content.empty())
    {
        content_query_result = nontransaction.exec_params(
            "SELECT body_content FROM \"mailBodies\""
        );
    }
    else
    {
        content_query_result = nontransaction.exec_params(
            "SELECT body_content FROM \"mailBodies\" "
            "WHERE body_content = $1"
            , nontransaction.esc(content)
        );
    }

    std::vector<std::string> info{};
    if (!content_query_result.empty())
    {
        for (auto&& row : content_query_result)
        {
            info.emplace_back(row.at("body_content").as<std::string>());
        }

        return info;
    }

    return std::vector<std::string>();
}

void PgMailDB::InsertEmail(const std::string_view sender, const std::string_view receiver,
                                const std::string_view subject, const std::string_view body)
{
    if (!IsConnected())
    {
        throw MailException("Connection with database lost or was manually already closed");
    }


    uint32_t sender_id, receiver_id, body_id;
    {
        try
        {
            {
                pqxx::nontransaction nontransaction(*m_conn);
                sender_id = RetriveUserId(sender, nontransaction);
                receiver_id = RetriveUserId(receiver, nontransaction);
                body_id = InsertEmailContent(body, nontransaction);
            }
        }
        catch (const std::exception& e)
        {
            throw MailException("Given value doesn't exist in database.");
        }
    }
    
    try {
        pqxx::work transaction(*m_conn);
        PerformEmailInsertion(sender_id, receiver_id, subject, body_id, transaction);
        transaction.commit();
    }
    catch (const std::exception& e) {
        throw MailException(e.what());
    }
}

void PgMailDB::InsertEmail(const std::string_view sender, const std::vector<std::string_view> receivers, const std::string_view subject, const std::string_view body)
{
    if (!IsConnected())
    {
        throw MailException("Connection with database lost or was manually already closed");
    }

    uint32_t sender_id, body_id;
    std::vector<uint32_t> receivers_id;
    {
        try
        {
            {
                pqxx::nontransaction nontransaction(*m_conn);
                sender_id = RetriveUserId(sender, nontransaction);

                for (size_t i = 0; i < receivers.size(); i++) {
                    receivers_id.emplace_back(RetriveUserId(receivers[i], nontransaction));
                }
                
                body_id = InsertEmailContent(body, nontransaction);
            }
        }
        catch (const std::exception& e)
        {
            throw MailException("Given value doesn't exist in database.");
        }
    }

    try {
        pqxx::work transaction(*m_conn);
        for (size_t i = 0; i < receivers_id.size(); i++) {
            PerformEmailInsertion(sender_id, receivers_id[i], subject, body_id, transaction);
        }
        transaction.commit();
    }
    catch (const std::exception& e) {
        throw MailException(e.what());
    }
}

std::vector<Mail> PgMailDB::RetrieveEmails(const std::string_view user_name, bool should_retrieve_all) const
{
    pqxx::nontransaction ntx(*m_conn);

    uint32_t user_id = RetriveUserId(user_name, ntx);

    std::string query =
        "WITH filtered_emails AS ( "
        "    SELECT sender_id, subject, mail_body_id, sent_at "
        "    FROM \"emailMessages\" "
        "    WHERE recipient_id = " +
        ntx.quote(user_id) + 
        (should_retrieve_all ? "": " AND is_received = FALSE")+
        ")"
        "SELECT u.user_name AS sender_name, f.subject, m.body_content "
        "FROM filtered_emails AS f "
        "LEFT JOIN users AS u ON u.user_id = f.sender_id "
        "LEFT JOIN \"mailBodies\" AS m ON m.mail_body_id = f.mail_body_id "
        "ORDER BY f.sent_at DESC; ";

    std::vector<Mail> resutl_mails;

    for (auto& [sender, subject, body] : ntx.query<std::string, std::string, std::string>(query))
    {
        resutl_mails.emplace_back(user_name, sender, subject, body);
    }

    return resutl_mails;
}

void PgMailDB::MarkEmailsAsReceived(const std::string_view user_name)
{

    pqxx::work tx(*m_conn);

    uint32_t user_id = RetriveUserId(user_name, tx);

    tx.exec_params(
        "UPDATE \"emailMessages\" "
        "SET is_received = TRUE "
        "WHERE recipient_id = $1 "
        "AND is_received = FALSE"
        , user_id);

    tx.commit();
}

bool PgMailDB::UserExists(const std::string_view user_name)
{
    pqxx::nontransaction ntx(*m_conn);
    try 
    {
        ntx.exec_params1("SELECT 1 FROM users WHERE host_id = $1 AND user_name = $2", m_host_id, user_name);
        return true;
    }
    catch(pqxx::unexpected_rows &e)
    {
        return false;
    }
}

void PgMailDB::DeleteEmail(const std::string_view user_name)
{
    if (!IsConnected())
    {
        throw MailException("Connection with database lost or was manually already closed");
    }

    uint32_t user_info;
    {
        try {
            pqxx::nontransaction nontransaction(*m_conn);
            user_info = RetriveUserId(user_name, nontransaction);
        }
        catch (const std::exception& e)
        {
            throw MailException("Given value doesn't exist in database.");
        }
    }

    try
    {
        pqxx::work transaction(*m_conn);
        transaction.exec_params(
            "DELETE FROM \"emailMessages\" "
            "WHERE sender_id = $1 OR recipient_id = $1"
            , user_info
        );
        transaction.commit();
    }
    catch(const std::exception& e)
    {
        throw MailException(e.what());
    }
}

void PgMailDB::DeleteUser(const std::string_view user_name, const std::string_view password)
{
    if (!IsConnected())
    {
        throw MailException("Connection with database lost or was manually already closed");
    }
    
    try 
    {
        Login(user_name, password);
        DeleteEmail(user_name);
    }
    catch (const MailException& e) {
        throw MailException(e.what());
    }

    try
    {
        pqxx::work transaction(*m_conn);

        std::string hashed_password = transaction.query_value<std::string>("SELECT password_hash FROM users "
            "WHERE user_name = " + transaction.quote(user_name) 
            +  " AND host_id = " + transaction.quote(m_host_id)
            );

        transaction.exec_params(
            "DELETE FROM users "
            "WHERE user_name = $1 AND password_hash = $2 AND host_id = $3"
            , transaction.esc(user_name), transaction.esc(hashed_password), m_host_id
        );
        transaction.commit();
    }
    catch (const std::exception& e)
    {
        throw MailException(e.what());
    }
} 

uint32_t PgMailDB::InsertEmailContent(const std::string_view content, pqxx::transaction_base& transaction)
{
    if (!IsConnected())
    {
        throw MailException("Connection with database lost or was manually already closed");
    }

    pqxx::result body_id;
    try {
        body_id = transaction.exec_params(
            "INSERT INTO \"mailBodies\" (body_content)VALUES($1) RETURNING mail_body_id"
            , content
        );
        transaction.commit();
    }
    catch (const std::exception& e) {
        throw MailException(e.what());
    }

    return body_id[0].at("mail_body_id").as<uint32_t>();
}

uint32_t PgMailDB::RetriveUserId(const std::string_view user_name, pqxx::transaction_base &ntx) const
{
    try
    {
        return ntx.query_value<uint32_t>("SELECT user_id FROM users WHERE user_name = " + ntx.quote(user_name) 
                                         + "  AND host_id = " + ntx.quote(m_host_id));
    }
    catch (pqxx::unexpected_rows &e)
    {
        throw MailException("User doesn't exist");
    };
}  

void PgMailDB::PerformEmailInsertion(const uint32_t sender_id, const uint32_t receiver_id,
                                const std::string_view subject, const uint32_t body_id, pqxx::transaction_base& transaction)
{
    try {
        transaction.exec_params(
            "INSERT INTO \"emailMessages\" (sender_id, recipient_id, subject, mail_body_id, is_received) "
            "VALUES ($1, $2, $3, $4, false) "
            , sender_id, receiver_id,
            subject, body_id
        );
    }
    catch (const std::exception& e) {
        throw MailException(e.what());
    }
}
}