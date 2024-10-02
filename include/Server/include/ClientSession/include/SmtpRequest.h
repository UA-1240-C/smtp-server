#ifndef SmtpRequest_H
#define SmtpRequest_H

#include <string>

namespace ISXSmtpRequest {
enum class SmtpCommand {
    EHLO,
    HELP,
    NOOP,
    STARTTLS,
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
};
    
}; // namespace ISXCState

#endif // SmtpRequest_H
