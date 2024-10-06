#pragma once
#include "MailMessageBuilder.h"

namespace ISXMM
{
namespace MailMessageFormatter
{
    const inline std::string boundary = "NEXT_PART_My4xNDE1OTI2NTM1";
    std::string MailFrom(const MailAddress& from);
    std::string MailTo(const std::vector<MailAddress>& to);
    std::string MailCc(const std::vector<MailAddress>& cc);
    std::string MailHeaders(const MailMessage& message);
    std::string MailBody(const MailMessage& message);
    std::string MailAttachmentHeaders(const MailAttachment& attachment, const std::string& filetype = "application/octet-stream");
}; // namespace MailMessageFormatter
} // namespace ISXMM