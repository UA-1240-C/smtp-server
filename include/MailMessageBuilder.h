#pragma once

#include <string>

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
        std::string GetBody();
        MailMessageBuilder& AddAttachment(const std::string &path);
        MailMessage Build();

    private:
        MailMessage mail_message_;
    };
}