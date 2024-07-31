#pragma once

#include <sstream>
#include <string>
#include <vector>

#include "MailAttachment.h"
#include "MailMessage.h"

namespace ISXSC {

class MailMessageBuilder {
public:
    MailMessageBuilder& SetFrom(const std::string& email,
                                const std::string& name = "");
    MailMessageBuilder& AddTo(const std::string& email,
                              const std::string& name = "");
    MailMessageBuilder& AddCc(const std::string& email,
                              const std::string& name = "");
    MailMessageBuilder& AddBcc(const std::string& email,
                               const std::string& name = "");
    MailMessageBuilder& SetSubject(const std::string& subject);
    MailMessageBuilder& SetBody(const std::string& body);
    MailMessageBuilder& AddData(const std::string& data);
    std::string GetBody() const;
    MailMessageBuilder& AddAttachment(const std::string& path);
    MailMessage Build();

private:
    MailMessage mail_message_;
    std::stringstream body_stream_;  // Added to store body data
};

}  // namespace ISXSC
