#pragma once

#include <vector>
#include <string>

#include "MailException.h"

namespace ISXMailDB
{

struct User
{
    User(const std::string& user_name, const std::string& user_password, const std::string& host_name)
        : user_name{user_name}, user_password{user_password}, host_name{host_name} {}

    std::string GetPasswordHash() const {
        return user_password;
    }

    std::string user_name;
    std::string user_password;
    std::string host_name;
};

struct Mail
{
    std::string sender;
    std::string subject;
    std::string body;

    Mail(std::string sender, std::string subject, std::string body)
        : sender(std::move(sender)), subject(std::move(subject)), body(std::move(body)) {}
};

class IMailDB
{
public:
    IMailDB(const std::string_view host_name)
    {
        if (host_name.empty())
        {
            throw MailException("Host name couldn't be empty"); // change
        }
        m_host_name = host_name;
    }
    virtual ~IMailDB() = default;

    IMailDB(const IMailDB&) = delete;
    IMailDB& operator=(const IMailDB&) = delete;

    IMailDB(IMailDB&&) = delete;
    IMailDB& operator=(IMailDB&&) = delete;

    // TODO: Viacheslav
    virtual void Connect(const std::string &connection_string) = 0;
    virtual void Disconnect() = 0;
    virtual bool IsConnected() const = 0;

    // TODO: Denys
    virtual void SignUp(const std::string_view user_name, const std::string_view hash_password) = 0;
    virtual void Login(const std::string_view user_name, const std::string_view hash_password) = 0;

    // TODO: Viacheslav
    virtual std::vector<User> RetrieveUserInfo(const std::string_view user_name = "") = 0;
    virtual void InsertEmailContent(const std::string_view content) = 0;
    virtual std::vector<std::string> RetrieveEmailContentInfo(const std::string_view content = "") = 0;
    virtual void InsertEmail(const std::string_view sender, const std::string_view receiver,
                                const std::string_view subject, const std::string_view body) = 0;
    virtual void InsertEmail(const std::string_view sender, std::vector<std::string_view> receivers,
                                const std::string_view subject, const std::string_view body) = 0;

    // TODO: Denys
    virtual std::vector<Mail> RetrieveEmails(const std::string_view user_name, bool should_retrieve_all = false) const = 0;
    virtual void MarkEmailsAsReceived(const std::string_view user_name) = 0;
    virtual bool UserExists(const std::string_view user_name) = 0;

    // TODO: Viacheslav
    virtual void DeleteEmail(const std::string_view user_name) = 0;
    virtual void DeleteUser(const std::string_view user_name, const std::string_view hash_password) = 0;

protected:
    virtual void InsertHost(const std::string_view host_name) = 0;

    std::string m_host_name;
    uint32_t m_host_id;
};

}