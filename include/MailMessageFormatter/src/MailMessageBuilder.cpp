#include "MailMessageBuilder.h"

#include "MailAddress.h"
#include "MailMessage.h"
#include "MailAttachment.h"

namespace ISXSC
{
    MailMessageBuilder& MailMessageBuilder::SetFrom(const std::string& email, const std::string& name)
    {
        m_from = { email, name };
        return *this;
    }

    MailMessageBuilder& MailMessageBuilder::AddTo(const std::string& email, const std::string& name)
    {
        m_to.push_back({ email, name });
        return *this;
    }

    MailMessageBuilder& MailMessageBuilder::AddCc(const std::string& email, const std::string& name)
    {
        m_cc.push_back({ email, name });
        return *this;
    }

    MailMessageBuilder& MailMessageBuilder::AddBcc(const std::string& email, const std::string& name)
    {
        m_bcc.push_back({ email, name });
        return *this;
    }

    MailMessageBuilder& MailMessageBuilder::SetSubject(const std::string& subject)
    {
        m_subject = subject;
        return *this;
    }

    MailMessageBuilder& MailMessageBuilder::SetBody(const std::string& body)
    {
        m_body = body;
        return *this;
    }

    MailMessageBuilder& MailMessageBuilder::AddAttachment(const std::string &path)
    {
        try
        {
            attachments.push_back({ path }); 
        }
        catch(const std::exception& e)
        {
            throw e;
        }
        
        return *this;
    }

    MailMessage MailMessageBuilder::Build()
    {
        if (m_from.get_address().empty() || m_to.empty())
        {
            throw std::runtime_error("Not all required fields are filled");
        }

        MailMessage message { m_from, m_to, m_cc, m_bcc, m_subject, m_body, attachments };
        return message;
    }
}