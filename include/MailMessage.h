#pragma once

#include <vector>
#include <string>
#include "MailAddress.h"
#include "MailAttachment.h"

namespace ISXSC
{
    struct MailMessage
    {
        MailAddress from_;
        std::vector<MailAddress> to_;
        std::vector<MailAddress> cc_;
        std::vector<MailAddress> bcc_;
        std::string subject_;
        std::string body_;
        std::vector<MailAttachment> attachments_;
    };
}