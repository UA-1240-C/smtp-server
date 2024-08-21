#include "MailMessageFormatter.h"

#include <vector>
#include <string>
#include <sstream>
#include <iostream>

#include "MailMessage.h"
#include "MailAddress.h"
#include "MailAttachment.h"

namespace ISXMM {
std::string MailMessageFormatter::MailFrom(const MailAddress& from) 
{
    if (from.get_name().empty()) 
    {
        return "From: " + from.get_address() + "\r\n";
    }
    return "From: " + from.get_name() + " <" + from.get_address() + ">\r\n";
}

std::string MailMessageFormatter::MailTo(const std::vector<MailAddress>& to) 
{
    std::ostringstream to_stream;
    to_stream << "To: ";
    for (const auto& address : to)
     {
        if (address.get_name().empty()) 
        {
            to_stream << address.get_address() << ", ";
        }
        else
        {
            to_stream << address.get_name() << " <" << address.get_address() << ">, ";
        }
    }
    std::string to_string = to_stream.str();
    to_string.erase(to_string.length() - 2, 2);
    to_string += "\r\n";

    return to_string;
}

std::string MailMessageFormatter::MailCc(const std::vector<MailAddress>& cc) 
{
    if (cc.empty()) {return ""; }

    std::ostringstream cc_stream;
    cc_stream << "Cc: ";
    for (const auto& address : cc) {
        if (address.get_name().empty()) 
        {
            cc_stream << address.get_address() << ", ";
        }
        else
        {
            cc_stream << address.get_name() << " <" << address.get_address() << ">, ";
        }
    }
    std::string cc_string = cc_stream.str();
    cc_string.erase(cc_string.length() - 2, 2);
    cc_string += "\r\n";

    return cc_string;
}

std::string MailMessageFormatter::MailHeaders(const MailMessage& message) 
{
    std::ostringstream headers;
    headers << MailFrom(message.from)
            << MailTo(message.to)
            << MailCc(message.cc)
            << "Subject: " + message.subject + "\r\n";

    if (!message.attachments.empty()) 
    {
        headers << "MIME-Version: 1.0\r\n"
                << "Content-Type: multipart/mixed; boundary=\"" + boundary + "\"\r\n";
    }

    headers << "\r\n";
    return headers.str();
}

std::string MailMessageFormatter::MailBody(const MailMessage& message) 
{
    std::ostringstream body;

    if (!message.attachments.empty()) 
    {
        body << "--" + boundary + "\r\n"
             << "Content-Type: text/plain; charset=\"UTF-8\"\r\n"
             << "Content-Transfer-Encoding: 7bit\r\n\r\n";
    }

    body << message.body + "\r\n";
    return body.str();
}

std::string MailMessageFormatter::MailAttachmentHeaders(const MailAttachment& attachment, const std::string& filetype)
{
    std::ostringstream formatted_attachments;
    formatted_attachments  << "Content-Type: " << filetype << "; name=\"" + attachment.get_name() + "\"\r\n"
                              << "Content-Transfer-Encoding: base64\r\n"
                              << "Content-Disposition: attachment; filename=\"" + attachment.get_name() + "\"\r\n\r\n";
                              
    return formatted_attachments.str();
}
} // namespace ISXMM