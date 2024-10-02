#include "SmtpRequest.h"
#include <stdexcept>
#include <algorithm>

namespace ISXSmtpRequest
{
SmtpRequest RequestParser::Parse(const std::string& request)
{
    SmtpRequest smtp_request;
    smtp_request.data = request;

    if (request.find("EHLO") == 0)
    {
        smtp_request.command = SmtpCommand::EHLO;
    }
    else if (request.find("HELP") == 0)
    {
        smtp_request.command = SmtpCommand::HELP;
    }
    else if (request.find("NOOP") == 0)
    {
        smtp_request.command = SmtpCommand::NOOP;
    }
    else if (request.find("STARTTLS") == 0)
    {
        smtp_request.command = SmtpCommand::STARTTLS;
    }
    else if (request.find("AUTH") == 0)
    {
        smtp_request.command = SmtpCommand::AUTH;
    }
    else if (request.find("MAIL FROM") == 0)
    {
        smtp_request.command = SmtpCommand::MAILFROM;
    }
    else if (request.find("RCPT TO") == 0)
    {
        smtp_request.command = SmtpCommand::RCPTTO;
    }
    else if (request.find("DATA") == 0)
    {
        smtp_request.command = SmtpCommand::DATA;
    }
    else if (request.find("RSET") == 0)
    {
        smtp_request.command = SmtpCommand::RSET;
    }
    else if (request.find("QUIT") == 0)
    {
        smtp_request.command = SmtpCommand::QUIT;
    }
    else {
        throw std::runtime_error("Invalid SMTP command.");
    }

    return smtp_request;
}

std::string RequestParser::ExtractUsername(std::string auth_data)
{
    auth_data.erase(std::remove(auth_data.begin(), auth_data.end(), ' '), auth_data.end());
    if (!auth_data.empty() && auth_data.front() == '<' && auth_data.back() == '>')
    {
        return auth_data.substr(1, auth_data.size() - 2);
    }

    return std::string();
}
} // namespace ISXSmtpRequest
