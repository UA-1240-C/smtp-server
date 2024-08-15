#pragma once

#include <vector>
#include <string>
#include <cstdint>

#include "MailException.h"

namespace ISXMailDB
{

struct User
{
    User(std::string user_name, std::string user_password, std::string host_name)
        : user_name{std::move(user_name)}, user_password{std::move(user_password)}, host_name{std::move(host_name)}
    {
    }

    std::string user_name;
    std::string user_password;
    std::string host_name;
};

struct Mail
{
    Mail(const std::string_view recipient, const std::string_view sender,
         const std::string_view subject, const std::string_view body)
        : recipient(recipient), sender(sender), 
          subject(subject), body(body)
    {
    }
    
    std::string recipient;
    std::string sender;
    std::string subject;
    std::string body;
};

std::ostream& operator<<(std::ostream& os, const Mail& mail);


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

    IMailDB(const IMailDB&);
    IMailDB& operator=(const IMailDB&) = delete;

    IMailDB(IMailDB&&) = delete;
    IMailDB& operator=(IMailDB&&) = delete;

    virtual void Connect(const std::string &connection_string) = 0;
    virtual void Disconnect() = 0;
    virtual bool IsConnected() const = 0;

    virtual void SignUp(const std::string_view user_name, const std::string_view password) = 0;
    virtual void Login(const std::string_view user_name, const std::string_view password) = 0;

    virtual std::vector<User> RetrieveUserInfo(const std::string_view user_name) = 0;
    virtual std::vector<std::string> RetrieveEmailContentInfo(const std::string_view content) = 0;
    virtual void InsertEmail(const std::string_view sender, const std::string_view receiver,
                                const std::string_view subject, const std::string_view body) = 0;
    virtual void InsertEmail(const std::string_view sender, const std::vector<std::string_view> receivers,
                                const std::string_view subject, const std::string_view body) = 0;

    virtual std::vector<Mail> RetrieveEmails(const std::string_view user_name, bool should_retrieve_all = false) const = 0;
    virtual void MarkEmailsAsReceived(const std::string_view user_name) = 0;
    virtual bool UserExists(const std::string_view user_name) = 0;

    virtual void DeleteEmail(const std::string_view user_name) = 0;
    virtual void DeleteUser(const std::string_view user_name, const std::string_view password) = 0;

protected:
    virtual void InsertHost(const std::string_view host_name) = 0;
    virtual std::string HashPassword(const std::string& password) = 0;
    virtual bool VerifyPassword(const std::string& password, const std::string& hashed_password) = 0;

    std::string m_host_name;
    uint32_t m_host_id;
};

}