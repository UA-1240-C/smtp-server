#pragma once

#include <vector>
#include <string>
#include <initializer_list>

#include "MailAddress.h"
#include "MailMessage.h"
#include "MailAttachment.h"

namespace ISXSC
{
    class MailMessageBuilder
    {
    public:
        MailMessageBuilder& SetFrom(const std::string& email, const std::string& name = "");
        MailMessageBuilder& AddTo(const std::string& email, const std::string& name = "");
        MailMessageBuilder& AddCc(const std::string& email, const std::string& name = "");
        MailMessageBuilder& AddBcc(const std::string& email, const std::string& name = "");
        MailMessageBuilder& SetSubject(const std::string& subject);
        MailMessageBuilder& SetBody(const std::string& body);
        MailMessageBuilder& AddAttachment(const std::string &path);
        MailMessage Build();

    private:
        MailAddress m_from;
        std::vector<MailAddress> m_to = {};
        std::vector<MailAddress> m_cc = {};
        std::vector<MailAddress> m_bcc = {};
        std::string m_subject;
        std::string m_body;
        std::vector<MailAttachment> attachments;
    };
}