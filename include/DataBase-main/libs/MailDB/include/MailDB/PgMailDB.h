#pragma once
#ifndef PG_MAIL_DB
#define PG_MAIL_DB

#include <memory>
#include <pqxx/pqxx>

#include "IMailDB.h"

namespace ISXMailDB
{

class PgMailDB : public IMailDB
{

public:
    PgMailDB(std::string_view host_name);
    virtual ~PgMailDB();

    // TODO: Viacheslav
    virtual void Connect(const std::string &connection_string) override;
    virtual void Disconnect() override;
    virtual bool IsConnected() const override;

    // TODO: Denys
    virtual void SignUp(const std::string_view user_name, const std::string_view hash_password) override;
    virtual void Login(const std::string_view user_name, const std::string_view hash_password) override;

    // TODO: Viacheslav
    virtual std::vector<User> RetrieveUserInfo(const std::string_view user_name = "") override;
    virtual void InsertEmailContent(const std::string_view content) override;
    virtual std::vector<std::string> RetrieveEmailContentInfo(const std::string_view content = "") override;
    virtual void InsertEmail(const std::string_view sender, const std::string_view receiver,
                                const std::string_view subject, const std::string_view body) override;
    virtual void InsertEmail(const std::string_view sender, std::vector<std::string_view> receivers,
                                const std::string_view subject, const std::string_view body) override;

    // TODO: Denys
    virtual std::vector<Mail> RetrieveEmails(const std::string_view user_name, bool should_retrieve_all = false) const override;
    virtual void MarkEmailsAsReceived(const std::string_view user_name) override;
    virtual bool UserExists(const std::string_view user_name) override;


    // TODO: Viacheslav
    virtual void DeleteEmail(const std::string_view user_name) override;
    virtual void DeleteUser(const std::string_view user_name, const std::string_view hash_password) override;

protected:
    virtual void InsertHost(const std::string_view host_name) override;

    uint32_t RetriveUserId(const std::string_view user_name, pqxx::transaction_base &ntx) const;

    std::unique_ptr<pqxx::connection> m_conn;
};

}

#endif  // PG_MAIL_DB