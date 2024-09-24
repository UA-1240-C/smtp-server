#include <ares.h>
#include <ares_dns.h>
#include <boost/asio.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <set>

#include <ares.h>
#include <ares_dns.h>
#include <boost/asio.hpp>

#include "CommandHandler.h"
#include "Logger.h"
#include "StandartSmtpResponses.h"

constexpr std::size_t MAILING_LIST_PREFIX_LENGTH = 5;

constexpr std::size_t USERNAME_START_INDEX = 5;
constexpr std::size_t SENDER_START_INDEX = 10;
constexpr std::size_t RECIPIENT_START_INDEX = 8;

constexpr std::size_t AUTH_PREFIX_LENGTH = 11;

constexpr std::size_t DELIMITER_OFFSET = 2;

namespace ISXCommandHandler
{

CommandHandler::CommandHandler( boost::asio::io_context& io_context,
                                boost::asio::ssl::context& ssl_context, 
                                ISXMailDB::PgManager& database_manager)
    : m_io_context(io_context), 
      m_ssl_context(ssl_context), 
      m_data_base(std::make_unique<PgMailDB>(database_manager))  
{
    Logger::LogDebug("Entering CommandHandler constructor");
    Logger::LogTrace("Constructor params: ssl_context");

    // Set SSL options
    Logger::LogDebug("Setting SSL options");
    m_ssl_context.set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 |
                              boost::asio::ssl::context::no_sslv3 | boost::asio::ssl::context::no_tlsv1 |
                              boost::asio::ssl::context::no_tlsv1_1);
    Logger::LogProd("SSL options set successfully.");

    Logger::LogDebug("Exiting CommandHandler constructor");
}

CommandHandler::~CommandHandler()
{
    Logger::LogDebug("Entering CommandHandler destructor");
    Logger::LogDebug("Exiting CommandHandler destructor");
}

void CommandHandler::ProcessLine(const std::string& line, SocketWrapper& socket_wrapper)
{
    Logger::LogDebug("Entering CommandHandler::ProcessLine");
    Logger::LogTrace("CommandHandler::HandleEhlo parameters: SocketWrapper&, const std::string&");

    if (line.find("EHLO") == 0 || line.find("HELO") == 0)
    {
        HandleEhlo(socket_wrapper);
    }
    else if (line.find("MAIL FROM:") == 0)
    {
        HandleMailFrom(socket_wrapper, line);
    }
    else if (line.find("RCPT TO:") == 0)
    {
        HandleRcptTo(socket_wrapper, line);
    }
    else if (line.find("DATA") == 0)
    {
        HandleData(socket_wrapper);
    }
    else if (line.find("QUIT") == 0)
    {
        HandleQuit(socket_wrapper);
    }
    else if (line.find("NOOP") == 0)
    {
        HandleNoop(socket_wrapper);
    }
    else if (line.find("RSET") == 0)
    {
        HandleRset(socket_wrapper);
    }
    else if (line.find("HELP") == 0)
    {
        HandleHelp(socket_wrapper);
    }
    else if (line.find("STARTTLS") == 0)
    {
        HandleStartTLS(socket_wrapper);
    }
    else if (line.find("AUTH PLAIN") == 0)
    {
        HandleAuth(socket_wrapper, line);
    }
    else if (line.find("REGISTER") == 0)
    {
        HandleRegister(socket_wrapper, line);
    }
    else if (line.find("Access token:") == 0) 
    {
        HandleAccessToken(socket_wrapper, line);
    }
   
    else
    {
        try
        {
            socket_wrapper.WriteAsync(ToString(SmtpResponseCode::SYNTAX_ERROR)).get();
        }
        catch (const std::invalid_argument& e)
        {
            Logger::LogError("Invalid SMTP response code sent from CommandHandler::ProcessLine: " +
                             std::string(e.what()));
        }
        Logger::LogWarning("A client sent an unrecognized command to CommandHandler::ProcessLine");
    }

    Logger::LogDebug("Exiting CommandHandler::ProcessLine");
}

void CommandHandler::HandleEhlo(SocketWrapper& socket_wrapper)
{
    Logger::LogDebug("Entering CommandHandler::HandleEhlo");
    Logger::LogTrace("CommandHandler::HandleEhlo parameter: SocketWrapper reference");

    try
    {
        std::string to_write = "250-server.domain.com\r\n250-STARTTLS\r\n250-AUTH PLAIN\r\n" + ToString(SmtpResponseCode::OK);
        socket_wrapper.WriteAsync(to_write).get();
        Logger::LogProd("CommandHandler::HandleEhlo: Successfully sent EHLO response to client.");
    }
    catch (const std::exception& e)
    {
        Logger::LogError("CommandHandler::HandleEhlo: Exception caught while sending EHLO response: " +
                         std::string(e.what()));
    }

    Logger::LogDebug("Exiting CommandHandler::HandleEhlo");
}

void CommandHandler::HandleAccessToken(SocketWrapper &socket_wrapper, std::string line)
{
    try
    {
        socket_wrapper.WriteAsync(ToString(SmtpResponseCode::OK)).get();
        Logger::LogDebug(line);
        m_access_token = line = line.substr(13, line.size());
        Logger::LogDebug(line);
    }
    catch (const std::exception& e)
    {
        Logger::LogError("CommandHandler::HandleEhlo: Exception caught while sending EHLO response: " +
                         std::string(e.what()));
    }
}

void CommandHandler::HandleNoop(SocketWrapper& socket_wrapper)
{
    Logger::LogDebug("Entering CommandHandler::HandleNoop");
    Logger::LogTrace("CommandHandler::HandleNoop parameter: SocketWrapper reference");

    try
    {
        socket_wrapper.WriteAsync(ToString(SmtpResponseCode::OK)).get();
        Logger::LogProd("CommandHandler::HandleNoop: Successfully sent NOOP response to client.");
    }
    catch (const std::exception& e)
    {
        Logger::LogError("CommandHandler::HandleNoop: Exception caught while sending NOOP response: " +
                         std::string(e.what()));
    }
}

void CommandHandler::HandleRset(SocketWrapper& socket_wrapper)
{
    Logger::LogDebug("Entering CommandHandler::HandleRset");
    Logger::LogTrace("CommandHandler::HandleRset parameter: SocketWrapper reference");

    m_mail_builder = MailMessageBuilder();
    Logger::LogProd("CommandHandler::HandleRset: Successfully reset buffer and state table.");

    try
    {
        socket_wrapper.WriteAsync(ToString(SmtpResponseCode::OK)).get();
        Logger::LogProd("CommandHandler::HandleRset: Successfully sent RSET response to client.");
    }
    catch (const std::exception& e)
    {
        Logger::LogError("CommandHandler::HandleRset: Exception caught while sending RSET response: " +
                         std::string(e.what()));
    }
    Logger::LogDebug("Exiting CommandHandler::HandleRset");
}

void CommandHandler::HandleHelp(SocketWrapper& socket_wrapper)
{
    Logger::LogDebug("Entering CommandHandler::HandleHelp");
    Logger::LogTrace("CommandHandler::HandleHelp parameter: SocketWrapper reference");

    const std::string supported_commands =
        "HELO, MAIL FROM, RCPT TO, DATA, "
        "QUIT, NOOP, RSET, HELP, "
        "STARTTLS, AUTH PLAIN, REGISTER\r\n";
    try
    {
        socket_wrapper.WriteAsync(ToString(SmtpResponseCode::HELP_MESSAGE) + " :" + supported_commands).get();
        Logger::LogProd("CommandHandler::HandleHelp: Successfully sent HELP response to client.");
    }
    catch (const std::exception& e)
    {
        Logger::LogError("CommandHandler::HandleHelp: Exception caught while sending HELP response: " +
                         std::string(e.what()));
    }
    Logger::LogDebug("Exiting CommandHandler::HandleHelp");
}

void CommandHandler::HandleQuit(SocketWrapper& socket_wrapper)
{
    Logger::LogDebug("Entering CommandHandler::HandleQuit");
    Logger::LogTrace("CommandHandler::HandleQuit parameter: SocketWrapper reference");

    try
    {
        socket_wrapper.WriteAsync(ToString(SmtpResponseCode::CLOSING_TRANSMISSION_CHANNEL)).get();
        Logger::LogProd("CommandHandler::HandleQuit: Successfully sent QUIT response to client.");
    }
    catch (const std::exception& e)
    {
        Logger::LogError("CommandHandler::HandleQuit: Exception caught while sending QUIT response: " +
                         std::string(e.what()));
        return;
    }

    socket_wrapper.Close();
    Logger::LogProd("Connection closed by client.");
    Logger::LogDebug("Exiting CommandHandler::HandleQuit");

    throw std::runtime_error("Client disconnected");
}

void CommandHandler::HandleMailFrom(SocketWrapper& socket_wrapper, const std::string& line)
{
    Logger::LogDebug("Entering CommandHandler::HandleMailFrom");
    Logger::LogTrace("CommandHandler::HandleMailFrom parameters: SocketWrapper reference, line: " + line);

    std::string sender = line.substr(10);
    Logger::LogDebug("Parsed sender: " + sender);

    sender.erase(std::remove(sender.begin(), sender.end(), ' '), sender.end());
    if (!sender.empty() && sender.front() == '<' && sender.back() == '>')
    {
        sender = sender.substr(1, sender.size() - 2);
    }

    try
    {
        if (m_data_base->get_user_name() != sender)
        {
            Logger::LogProd("Sender must be logged in");
            socket_wrapper
                .WriteAsync(ToString(SmtpResponseCode::INVALID_EMAIL_ADDRESS) +
                                   " : sender address doesn't exist.")
                .get();
        }
        else
        {
            m_mail_builder.set_from(sender);
            Logger::LogProd("Sender address set successfully: " + sender);
            socket_wrapper.WriteAsync(ToString(SmtpResponseCode::OK)).get();
        }
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception in CommandHandler::HandleMailFrom while processing sender: " +
                         std::string(e.what()));
    }
    Logger::LogDebug("Exiting CommandHandler::HandleMailFrom");
}

void CommandHandler::HandleRcptTo(SocketWrapper& socket_wrapper, const std::string& line)
{
    Logger::LogDebug("Entering CommandHandler::HandleRcptTo");
    Logger::LogTrace("CommandHandler::HandleMailFrom parameters: SocketWrapper reference, line: " + line);

    std::string recipient = line.substr(RECIPIENT_START_INDEX);
    Logger::LogTrace("Parsed recipient: " + recipient);

    recipient.erase(std::remove(recipient.begin(), recipient.end(), ' '), recipient.end());
    if (recipient.front() == '<' && recipient.back() == '>')
    {
        recipient = recipient.substr(1, recipient.size() - 2);
    }
    try
    {
        if (!m_data_base->UserExists(recipient))
        {
            Logger::LogProd("Recipient address does not exist: " + recipient);
            socket_wrapper
                .WriteAsync(ToString(SmtpResponseCode::INVALID_EMAIL_ADDRESS) +
                                   " : recipient address doesn't exist.")
                .get();
            return;
        }
        m_mail_builder.add_to(recipient);
        Logger::LogProd("Recipient address set successfully: " + recipient);
        socket_wrapper.WriteAsync(ToString(SmtpResponseCode::OK)).get();
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception in CommandHandler::HandleRcptTo while processing recipient : " +
                         std::string(e.what()));
    }
    Logger::LogDebug("Exiting CommandHandler::HandleRcptTo");
}

void CommandHandler::HandleData(SocketWrapper& socket_wrapper)
{
    Logger::LogDebug("Entering CommandHandler::HandleData");
    Logger::LogTrace("CommandHandler::HandleData parameters: SocketWrapper reference");

    try
    {
        socket_wrapper.WriteAsync(ToString(SmtpResponseCode::START_MAIL_INPUT)).get();
        Logger::LogProd("Sent response for DATA command, waiting for data.");

        m_in_data = true;
        std::string body{};
        while (m_in_data)
        {
            std::string data_message;
            ReadData(socket_wrapper, data_message);
            ProcessDataMessage(socket_wrapper, data_message, body);
        }

        Logger::LogProd("Data handling complete.");
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception in CommandHandler::HandleData :" + std::string(e.what()));
    }

    Logger::LogDebug("Exiting CommandHandler::HandleData");
}

void CommandHandler::ReadData(SocketWrapper& socket_wrapper, std::string& data_message)
{
    Logger::LogDebug("Entering CommandHandler::ReadData");
    Logger::LogTrace(
        "CommandHandler::ReadData parameters: "
        "SocketWrapper reference, std::string reference " +
        data_message);

    try
    {
        std::string buffer = socket_wrapper.ReadAsync(MAX_LENGTH).get();
        data_message.append(buffer);

        Logger::LogProd("Received data: " + buffer);
    }
    catch (const boost::system::system_error& e)
    {
        // Handle specific boost system errors (e.g., client disconnection)
        Logger::LogError("System error in CommandHandler::ReadData(Client disconnected): " + std::string(e.what()));
        m_in_data = false;
    }
    catch (const std::exception& e)
    {
        // Handle other exceptions
        Logger::LogError("Exception in CommandHandler::ReadData: " + std::string(e.what()));
        throw;
    }

    Logger::LogDebug("Exiting CommandHandler::ReadData");
}

void CommandHandler::ProcessDataMessage(SocketWrapper& socket_wrapper, std::string& data_message, std::string& body)
{
    Logger::LogDebug("Exiting CommandHandler::ProcessDataMessage");
    Logger::LogTrace(
        "CommandHandler::ProcessDataMessage parameters: "
        "SocketWrapper reference, std::string reference " +
        data_message);

    std::size_t last_pos{}, start_pos{};
    try
    {
        while ((last_pos = data_message.find("\r\n", last_pos)) != std::string::npos)
        {
            std::string line = data_message.substr(start_pos, last_pos - start_pos);
            last_pos += DELIMITER_OFFSET;

            if (line == ".")
            {
                Logger::LogProd("End-of-data sequence detected, exiting data read loop.");

                m_mail_builder.set_body(body + "\r\n");
                Logger::LogProd("Set body: " + body);

                HandleEndOfData(socket_wrapper);
                m_in_data = false;
                break;
            }

            if (line.find("Subject: ") == 0)
            {
                std::string subject = line.substr(9);
                m_mail_builder.set_subject(subject);
                Logger::LogDebug("Subject set to: " + subject);
            }
            else
            {
                body += line + "\r\n";
                Logger::LogProd("Appended to body: " + line);
            }

            start_pos = last_pos;
        }
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception in CommandHandler::ProcessDataMessage: " + std::string(e.what()));
        throw;
    }
    Logger::LogDebug("Exiting CommandHandler::ProcessDataMessage");
}

void CommandHandler::HandleEndOfData(SocketWrapper& socket_wrapper) {
    Logger::LogDebug("Entering CommandHandler::HandleEndOfData");
    Logger::LogTrace("CommandHandler::ProcessDataMessage parameters: SocketWrapper reference");

    m_in_data = false;
    try {
        MailMessage message = m_mail_builder.Build();
        if (message.from.get_address().empty() || message.to.empty()) {
            auto future_response =
                socket_wrapper.WriteAsync(ToString(SmtpResponseCode::REQUIRED_FIELDS_MISSING));
            Logger::LogWarning("Required fields missing in mail message.");
        } else {
            socket_wrapper.WriteAsync(ToString(SmtpResponseCode::OK)).get();
            Logger::LogProd("Sent 250 OK response for end of data.");
            
            SaveMailToDatabase(message);
            Logger::LogProd("Mail message saved successfully.");
            
            Logger::LogDebug(m_access_token);
            std::string oauth2_token = "user=egorchampion235@gmail.com\x01"
                              "auth=Bearer " + Base64Decode(m_access_token) + "\x01\x01";
            
            Logger::LogDebug(Base64Decode(m_access_token));
            
            ForwardMail(message, oauth2_token);

            Logger::LogProd("Mail message sent successfully.");
        }
    }
    catch (const std::exception& e) {
        Logger::LogError("Exception in CommandHandler::HandleEndOfData: " + std::string(e.what()));
        throw;
    }

    m_mail_builder = MailMessageBuilder();
    Logger::LogProd("MailBuilder reset after handling end of data.");

    Logger::LogDebug("Exiting CommandHandler::HandleEndOfData");
}


void CommandHandler::ConnectToSmtpServer(SocketWrapper& socket_wrapper) {
    socket_wrapper.ResolveAndConnectAsync(m_io_context, "smtp.gmail.com", "587").get();
    Logger::LogProd("Connected to smtp.gmail.com on port 587");

    std::string response = socket_wrapper.ReadAsync(MAX_LENGTH).get();
    std::cout << "Server response: " << response << std::endl;
    SendSmtpCommand(socket_wrapper, "HELO example.com");
}

std::string CommandHandler::SendSmtpCommand(SocketWrapper& socket_wrapper, const std::string& command) {
    std::string command_with_endl = command + "\r\n";
    socket_wrapper.WriteAsync(command_with_endl).get();
    std::string response = socket_wrapper.ReadAsync(MAX_LENGTH).get();
    std::cout << "Server response: " << response << std::endl;
    return response;
}

void CommandHandler::ForwardMail(const MailMessage& message, const std::string& oauth2_token) {
    Logger::LogDebug("Entering CommandHandler::SendMail");

    try {
        auto tcp_socket = std::make_shared<TcpSocket>(m_io_context);
        SocketWrapper socket_wrapper(tcp_socket);

	ConnectToSmtpServer(socket_wrapper);

        SendSmtpCommand(socket_wrapper, "STARTTLS");
        
        boost::asio::ssl::context context(boost::asio::ssl::context::tls_client);

        socket_wrapper.PerformTlsHandshake(boost::asio::ssl::stream_base::handshake_type::client, context).get();

        // XOAUTH2
        std::string response = SendSmtpCommand(socket_wrapper, "AUTH XOAUTH2 " + Base64Encode(oauth2_token));

        if (response.substr(0, 3) == "235") {  // 235 = Authentication successful
            SendSmtpCommand(socket_wrapper, "MAIL FROM:<" + message.from.get_address() + ">");
            for (const auto& recipient : message.to) {
                SendSmtpCommand(socket_wrapper, "RCPT TO:<" + recipient.get_address() + ">");
            }
            SendSmtpCommand(socket_wrapper, "DATA");
            SendSmtpCommand(socket_wrapper, "Subject: " + message.subject + "\r\n" + message.body + "\r\n.");
            SendSmtpCommand(socket_wrapper, "QUIT");
        }

    } catch (const std::exception& e) {
        Logger::LogError("Exception in CommandHandler::SendMail: " + std::string(e.what()));
        throw;
    }

    Logger::LogDebug("Exiting CommandHandler::SendMail");
}

void CommandHandler::SaveMailToDatabase(const MailMessage& message)
{
    Logger::LogDebug("Entering CommandHandler::SaveMailToDatabase");
    Logger::LogTrace("CommandHandler::SaveMailToDatabase parameters: MailMessage");

    try
    {
        for (const auto& recipient : message.to)
        {
            try
            {
                m_data_base->InsertEmail(recipient.get_address(), message.subject,
                                         message.body);
                Logger::LogDebug("Body: " + message.body);
                Logger::LogDebug("subject: " + message.subject);
                Logger::LogProd("Email inserted into database for recipient: " + recipient.get_address());
            }
            catch (const std::exception& e)
            {
                Logger::LogError(
                    "Exception in CommandHandler::SaveMailToDatabase while inserting an email for recipient: " +
                    recipient.get_address() + ": " + e.what());
            }
        }
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception in CommandHandler::SaveMailToDatabase: " + std::string(e.what()));
        throw;
    }

    Logger::LogDebug("Exiting CommandHandler::SaveMailToDatabase");
}

void CommandHandler::HandleStartTLS(SocketWrapper& socket_wrapper)
{
    Logger::LogDebug("Entering CommandHandler::HandleStartTLS");
    Logger::LogTrace("CommandHandler::HandleStartTLS parameters: SocketWrapper reference");

    if (socket_wrapper.IsTls())
    {
        Logger::LogWarning("STARTTLS command received but already in TLS mode.");
        try
        {
            socket_wrapper.WriteAsync(ToString(SmtpResponseCode::BAD_SEQUENCE)).get();
        }
        catch (const std::exception& e)
        {
            Logger::LogError("Exception in CommandHandler::HandleStartTLS: " + std::string(e.what()));
        }
        Logger::LogDebug("Exiting CommandHandler::HandleStartTLS");
        return;
    }

    try
    {
        Logger::LogProd("Sending response to indicate readiness to start TLS.");
        socket_wrapper.WriteAsync("220 Ready to start TLS\r\n").get();

        Logger::LogDebug(socket_wrapper.IsTls() ? "true" : "false");

        Logger::LogDebug("Starting TLS handshake.");
        socket_wrapper.PerformTlsHandshake(boost::asio::ssl::stream_base::server, m_ssl_context).get();

        Logger::LogProd("STARTTLS handshake completed successfully.");
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception in CommandHandler::HandleStartTLS: " + std::string(e.what()));
        try
        {
            socket_wrapper.WriteAsync(ToString(SmtpResponseCode::TLS_TEMPORARILY_UNAVAILABLE)).get();
        }
        catch (const std::exception& send_e)
        {
            Logger::LogError("Exception in CommandHandler::HandleStartTLS: " + std::string(send_e.what()));
        }
    }

    Logger::LogDebug("Exiting CommandHandler::HandleStartTLS");
}

void CommandHandler::HandleAuth(SocketWrapper& socket_wrapper, const std::string& line)
{
    Logger::LogDebug("Entering CommandHandler::HandleAuth");
    Logger::LogTrace(
        "CommandHandler::HandleAuth parameters: SocketWrapper reference, "
        "std::string reference " +
        line);

    try
    {
        // Decode the username and password from the AUTH command line
        auto [username, password] = DecodeAndSplitPlain(line.substr(AUTH_PREFIX_LENGTH));
        Logger::LogTrace("Decoded username: " + username);
        Logger::LogTrace("Decoded password: [hidden]");

        // Attempt to log in with the provided credentials
        try
        {
            m_data_base->Login(username, password);
            Logger::LogProd("User authenticated successfully");
            socket_wrapper.WriteAsync(ToString(SmtpResponseCode::AUTH_SUCCESSFUL)).get();
        }
        catch (const MailException& e)
        {
            Logger::LogError("MailException in CommandHandler::HandleAuth: " + std::string(e.what()));
            socket_wrapper.WriteAsync(ToString(SmtpResponseCode::AUTH_SUCCESSFUL)).get();
        }
       
      
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception in CommandHandler::HandleAuth: " + std::string(e.what()));
    }

    Logger::LogDebug("Exiting CommandHandler::HandleAuth");
}

void CommandHandler::HandleRegister(SocketWrapper& socket_wrapper, const std::string& line)
{
    Logger::LogDebug("Entering CommandHandler::HandleRegister");
    Logger::LogTrace(
        "CommandHandler::HandleRegister parameters: SocketWrapper reference, "
        "std::string reference " +
        line);

    try
    {
        auto [username, password] = DecodeAndSplitPlain(line.substr(9));
        Logger::LogProd("Decoded username: " + username);
        Logger::LogProd("Decoded password: [hidden]");

        try
        {
            m_data_base->SignUp(username, password);
            Logger::LogProd("User registered successfully");
            socket_wrapper.WriteAsync(ToString(SmtpResponseCode::REGISTER_SUCCESSFUL)).get();
        }
        catch(const MailException& e)
        {
            Logger::LogWarning("Registration failed: " + std::string(e.what()));
            socket_wrapper.WriteAsync(ToString(SmtpResponseCode::USER_ALREADY_EXISTS)).get();
        }
       
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception in CommandHandler::HandleRegister: " + std::string(e.what()));
    }

    Logger::LogDebug("Exiting CommandHandler::HandleRegister");
}

auto CommandHandler::DecodeAndSplitPlain(const std::string& encoded_data) -> std::pair<std::string, std::string>
{
    Logger::LogDebug("Entering CommandHandler::DecodeAndSplitPlain");
    Logger::LogTrace("CommandHandler::DecodeAndSplitPlain parameters: std::string reference" + encoded_data);

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
    Logger::LogDebug("Exiting CommandHandler::DecodeAndSplitPlain");

    return {username, password};
}
}  // namespace ISXCommandHandler
