#include "MailMessageBuilder.h"

#include "MailAddress.h"
#include "MailMessage.h"
#include "MailAttachment.h"

namespace ISXMM
{
MailMessageBuilder& MailMessageBuilder::set_from(const std::string& email, const std::string& name)
{
    m_from = { email, name };
    return *this;
}

MailMessageBuilder& MailMessageBuilder::add_to(const std::string& email, const std::string& name)
{
    m_to.push_back({ email, name });
    return *this;
}

MailMessageBuilder& MailMessageBuilder::add_cc(const std::string& email, const std::string& name)
{
    m_cc.push_back({ email, name });
    return *this;
}

MailMessageBuilder& MailMessageBuilder::add_bcc(const std::string& email, const std::string& name)
{
    m_bcc.push_back({ email, name });
    return *this;
}

MailMessageBuilder& MailMessageBuilder::set_subject(const std::string& subject)
{
    m_subject = subject;
    return *this;
}

MailMessageBuilder& MailMessageBuilder::set_body(const std::string& body)
{
    m_body = body;
    return *this;
}

MailMessageBuilder& MailMessageBuilder::add_attachment(const std::string &path)
{
    try
    {
        m_attachments.push_back({ path }); 
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
    MailMessage message { m_from, m_to, m_cc, m_bcc, m_subject, m_body, m_attachments };
    return message;
}
} // namespace ISXMM