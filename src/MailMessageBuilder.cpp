#include "MailMessageBuilder.h"
#include "MailAddress.h"
#include "MailMessage.h"
#include "MailAttachment.h"

namespace ISXSC
{
    MailMessageBuilder& MailMessageBuilder::SetFrom(const std::string& email, const std::string& name)
    {
        mail_message_.from_ = { email, name };
        return *this;
    }

    MailMessageBuilder& MailMessageBuilder::AddTo(const std::string& email, const std::string& name)
    {
        mail_message_.to_.emplace_back(email, name);
        return *this;
    }

    MailMessageBuilder& MailMessageBuilder::AddCc(const std::string& email, const std::string& name)
    {
        mail_message_.cc_.emplace_back(email, name);
        return *this;
    }

    MailMessageBuilder& MailMessageBuilder::AddBcc(const std::string& email, const std::string& name)
    {
        mail_message_.bcc_.emplace_back(email, name);
        return *this;
    }

    MailMessageBuilder& MailMessageBuilder::SetSubject(const std::string& subject)
    {
        mail_message_.subject_ = subject;
        return *this;
    }

    MailMessageBuilder& MailMessageBuilder::SetBody(const std::string& body)
    {
        mail_message_.body_ = body;
        return *this;
    }

    std::string MailMessageBuilder::GetBody()
    {
        return mail_message_.body_;
    }


    MailMessageBuilder& MailMessageBuilder::AddAttachment(const std::string &path)
    {
        try
        {
            mail_message_.attachments_.emplace_back(path);
        }
        catch(const std::exception& e)
        {
            throw e;
        }

        return *this;
    }

    MailMessage MailMessageBuilder::Build()
    {
        if (mail_message_.from_.get_address().empty() || mail_message_.to_.empty())
        {
            throw std::runtime_error("Not all required fields are filled");
        }

        return mail_message_;
    }
}