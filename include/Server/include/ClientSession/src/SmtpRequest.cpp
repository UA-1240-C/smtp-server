#include "SmtpRequest.h"
#include "MailMessageBuilder.h"
#include "Base64.h"
#include "Logger.h"

#include <stdexcept>
#include <algorithm>

using ISXMM::MailMessageBuilder;
using ISXBase64::Base64Decode;

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
    else if (request.find("REGISTER") == 0)
    {
        smtp_request.command = SmtpCommand::REGISTER;
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
        return auth_data.substr(1, auth_data.size() - DELIMITER_OFFSET);
    }

    return std::string();
}

std::string RequestParser::ExtractSubject(std::string data)
{
    std::size_t start_pos = data.find("Subject: ") + 9;
    std::size_t end_pos = data.find("\r\n", start_pos);
    
    if (start_pos == std::string::npos || end_pos == std::string::npos)
    {
        throw std::runtime_error("Subject not found.");
    }

    return data.substr(start_pos, end_pos - start_pos);
}

std::string RequestParser::ExtractBody(std::string data)
{
    std::size_t start_pos = data.find("\r\n\r\n") + 4;
    if (start_pos == std::string::npos)
    {
        throw std::runtime_error("Body not found.");
    }

    // Remove the last period from the body
    std::size_t last_pos = data.find("\r\n.", start_pos);

    if (last_pos == std::string::npos)
    {
        throw std::runtime_error("End-of-data sequence not found.");
    }

    return data.substr(start_pos, last_pos - start_pos);
}

std::pair<std::string, std::string> RequestParser::DecodeAndSplitPlain(const std::string& encoded_data)
{
    Logger::LogDebug("Entering ClientSession::DecodeAndSplitPlain");
    Logger::LogTrace("ClientSession::DecodeAndSplitPlain parameters: std::string reference" + encoded_data);

    // Decode Base64-encoded data
    std::string decoded_data;
    try
    {
        decoded_data = Base64Decode(encoded_data);
        Logger::LogDebug(decoded_data);
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Base64 decoding failed: " + std::string(e.what()));
        throw std::runtime_error("Base64 decoding failed.");
    }

    // Find the first null byte
    const size_t first_null = decoded_data.find('\0');
    if (first_null == std::string::npos)
    {
        Logger::LogError("Invalid PLAIN format: Missing first null byte.");
        throw std::runtime_error("Invalid PLAIN format: Missing first null byte.");
    }
    Logger::LogProd("First null byte at position: " + std::to_string(first_null));

    // Find the second null byte
    const size_t second_null = decoded_data.find('\0', first_null + 1);
    if (second_null == std::string::npos)
    {
        Logger::LogError("Invalid PLAIN format: Missing second null byte.");
        throw std::runtime_error("Invalid PLAIN format: Missing second null byte.");
    }
    Logger::LogProd("Second null byte at position: " + std::to_string(second_null));

    // Extract username and password
    std::string username = decoded_data.substr(first_null + 1, second_null - first_null - 1);
    std::string password = decoded_data.substr(second_null + 1);

    // Log the extracted username and password (password should be handled securely in real applications)
    Logger::LogProd("Extracted username: " + username);
    Logger::LogProd("Extracted password: [hidden]");

    Logger::LogTrace("Extracted username: " + username + ", Extracted password: [hidden]");
    Logger::LogDebug("Exiting ClientSession::DecodeAndSplitPlain");

    return {username, password};
}

} // namespace ISXSmtpRequest
