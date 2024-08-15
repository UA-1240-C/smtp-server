#pragma once

#include <memory>
#include <pqxx/pqxx>
#include <sodium/crypto_pwhash.h>

#include "IMailDB.h"

namespace ISXMailDB
{

class PgMailDB : public IMailDB
{

public:
    PgMailDB(std::string_view host_name);
    PgMailDB(const PgMailDB&);

    ~PgMailDB() override;

    void Connect(const std::string &connection_string) override;
    void Disconnect() override;
    bool IsConnected() const override;

    void SignUp(const std::string_view user_name, const std::string_view password) override;
    void Login(const std::string_view user_name, const std::string_view password) override;

    std::vector<User> RetrieveUserInfo(const std::string_view user_name) override;
    std::vector<std::string> RetrieveEmailContentInfo(const std::string_view content) override;
    void InsertEmail(const std::string_view sender, const std::string_view receiver,
                                const std::string_view subject, const std::string_view body) override;
    void InsertEmail(const std::string_view sender, const std::vector<std::string_view> receivers,
                                const std::string_view subject, const std::string_view body) override;

    std::vector<Mail> RetrieveEmails(const std::string_view user_name, bool should_retrieve_all = false) const override;
    void MarkEmailsAsReceived(const std::string_view user_name) override;
    bool UserExists(const std::string_view user_name) override;

    void DeleteEmail(const std::string_view user_name) override;
    void DeleteUser(const std::string_view user_name, const std::string_view password) override;

protected:
    void InsertHost(const std::string_view host_name) override;
    std::string HashPassword(const std::string& password) override;
    bool VerifyPassword(const std::string& password, const std::string& hashed_password) override;

    uint32_t InsertEmailContent(const std::string_view content, pqxx::transaction_base& transaction);
    uint32_t RetriveUserId(const std::string_view user_name, pqxx::transaction_base &ntx) const;

    void PerformEmailInsertion(const uint32_t sender_id, const uint32_t receiver_id,
                                const std::string_view subject, const uint32_t body_id, pqxx::transaction_base& transaction);

    std::unique_ptr<pqxx::connection> m_conn;

};

}