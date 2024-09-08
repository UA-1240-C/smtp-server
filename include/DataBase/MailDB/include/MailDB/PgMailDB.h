#pragma once

#include <memory>
#include <pqxx/pqxx>
#include <sodium/crypto_pwhash.h>

#include "IMailDB.h"
#include "ConnectionPool.h"
#include "ConnectionPoolWrapper.h"
#include "PgEmailsWriter.h"
#include "PgManager.h"
#include "EmailsInstance.h"

namespace ISXMailDB
{

/**
 * @class PgMailDB
 * @brief Concrete implementation of the IMailDB interface using PostgreSQL as the database backend.
 * 
 * This class provides an implementation of the `IMailDB` interface for interacting with a PostgreSQL database using pqxx library.
 * It handles operations such as connecting to the database, signing up and logging in users, and managing emails.
 */
class PgMailDB : public IMailDB
{

public:
    /**
     * @brief Constructs a PgMailDB.
     * 
     * @param manager The class that stores data required for initialization.
     */
    PgMailDB(PgManager& manager);

    /**
     * @brief Destructor for PgMailDB.
     */
    ~PgMailDB() override;

    /**
     * @copydoc IMailDB::SignUp
     */    
    void SignUp(const std::string_view user_name, const std::string_view password) override;
    /**
     * @copydoc IMailDB::Login
     */  
    void Login(const std::string_view user_name, const std::string_view password) override;

    /**
     * @copydoc IMailDB::Logout
     */  
    void Logout() override;

    /**
     * @copydoc IMailDB::RetrieveUserInfo
     */ 
    std::vector<User> RetrieveUserInfo(const std::string_view user_name) override;
    /**
     * @copydoc IMailDB::RetrieveEmailContentInfo
     */     
    std::vector<std::string> RetrieveEmailContentInfo(const std::string_view content) override;
    /**
     * @copydoc IMailDB::InsertEmail
     */     
    void InsertEmail(const std::string_view receiver, const std::string_view subject,
                     const std::string_view body) override;
    /**
     * @copydoc IMailDB::InsertEmail
     */      
    void InsertEmail(const std::vector<std::string_view> receivers, const std::string_view subject, 
                     const std::string_view body) override;

    /**
     * @copydoc IMailDB::RetrieveEmails
     */    
    std::vector<Mail> RetrieveEmails(bool should_retrieve_all = false) override;
    /**
     * @copydoc IMailDB::MarkEmailsAsReceived
     */    
    void MarkEmailsAsReceived() override;
    /**
     * @copydoc IMailDB::UserExists
     */    
    bool UserExists(const std::string_view user_name) override;

    /**
     * @copydoc IMailDB::DeleteEmail
     */    
    void DeleteEmail(const std::string_view user_name) override;
    /**
     * @copydoc IMailDB::DeleteUser
     */    
    void DeleteUser(const std::string_view user_name, const std::string_view password) override;

protected:
    /**
     * @copydoc IMailDB::InsertHost
     */ 
    void InsertHost(const std::string_view host_name) override;

    /**
     * @copydoc IMailDB::HashPassword
     */ 
    std::string HashPassword(const std::string& password) override;
    /**
     * @copydoc IMailDB::VerifyPassword
     */ 
    bool VerifyPassword(const std::string& password, const std::string& hashed_password) override;

    /**
     * @brief Inserts a new email body content into the "mailBodies" table.
     * 
     * This method inserts the provided email body content into the `mailBodies` table and retrieves the generated 
     * `mail_body_id` for the inserted content. The method ensures that the database connection is valid before attempting 
     * the insertion. If the connection is lost or manually closed, a `MailException` is thrown. Upon successful 
     * insertion, the method commits the transaction and returns the ID of the newly inserted email body content.
     * 
     * @param content A `std::string_view` representing the email body content to be inserted into the database.
     * @param transaction A `pqxx::transaction_base` object representing the ongoing database transaction. The transaction 
     *        must be active when calling this method.
     * 
     * @return uint32_t The ID of the newly inserted email body content, as retrieved from the `mailBodies` table.
     * 
     * @throws MailException If the database connection is lost or manually closed, or if any exception is thrown 
     *         during the execution of the database query.
     */
    uint32_t InsertEmailContent(const std::string_view content, pqxx::transaction_base& transaction);

    /**
     * @brief Retrieves the user ID for a given username from the "users" table.
     * 
     * @param user_name A `std::string_view` representing the username for which to retrieve the user ID.
     * @param ntx A `pqxx::transaction_base` object representing the ongoing database transaction. The transaction 
     *        must be active when calling this method.
     * 
     * @return uint32_t The ID of the user associated with the given username.
     * 
     * @throws MailException If the user does not exist in the database or if any exception is thrown during the 
     *         execution of the database query.
     */
    uint32_t RetriveUserId(const std::string_view user_name, pqxx::transaction_base &ntx) const;

    /**
     * @brief Inserts a new email record into the "emailMessages" table.
     * 
     * @param sender_id The ID of the user sending the email.
     * @param receiver_id The ID of the user receiving the email.
     * @param subject A `std::string_view` representing the subject of the email.
     * @param body_id The ID of the email body content.
     * @param transaction A `pqxx::transaction_base` object representing the ongoing database transaction. The 
     *        transaction must be active when calling this method.
     * 
     * @throws MailException If the insertion fails or if any exception is thrown during the execution of the 
     *         database query.
     */
    void PerformEmailInsertion(const uint32_t sender_id, const uint32_t receiver_id,
                               const std::string_view subject, const uint32_t body_id, pqxx::transaction_base& transaction);

    /**
     * @brief Check if there is a logged in user.
     * 
     * @throws MailException if the user is not logged in.
     */
    void CheckIfUserLoggedIn();

    std::string m_host_name; ///< The host name associated with the database.
    uint32_t m_host_id; ///< The host ID associated with the database.

    ConnectionPool<pqxx::connection>& m_connection_pool; ///< The connection pool of connections to the PostgreSQL database.
    std::shared_ptr<PgEmailsWriter> m_email_writer; ///< The connection pool of connections to the PostgreSQL database
};

}