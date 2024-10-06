#ifndef SMTP_REQUEST_H
#define SMTP_REQUEST_H

#include <string>
#include "MailMessage.h"

using ISXMM::MailMessage;

namespace ISXSmtpRequest {

constexpr std::size_t DELIMITER_OFFSET = 2;

enum class SmtpCommand {
    EHLO,
    HELP,
    NOOP,
    STARTTLS,
    REGISTER,
    AUTH,
    MAILFROM,
    RCPTTO,
    DATA,
    RSET,
    QUIT,
};

struct SmtpRequest {
    SmtpCommand command;
    std::string data;
};

class RequestParser {
private:
    RequestParser() = delete;
    RequestParser(const RequestParser&) = delete;
    RequestParser& operator=(const RequestParser&) = delete;
    RequestParser(RequestParser&&) = delete;
    ~RequestParser() = delete;

public:
    static SmtpRequest Parse(const std::string& request);

    static std::string ExtractUsername(std::string auth_data);
    static std::string ExtractSubject(std::string data);
    static std::string ExtractBody(std::string data);
    static std::pair<std::string, std::string> DecodeAndSplitPlain(const std::string& encoded_data);
};
    
}; // namespace ISXCState

#endif // !SMTP_REQUEST_H
