#include "CommandHandler.h"
#include "Logger.h"
#include "StandartSmtpResponses.h"
#include "MXResolver.h"

#include <ares.h>
#include <ares_dns.h>
#include <boost/asio.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <set>
#include <arpa/nameser.h>

constexpr std::size_t MAILING_LIST_PREFIX_LENGTH = 5;

constexpr std::size_t USERNAME_START_INDEX = 5;
constexpr std::size_t SENDER_START_INDEX = 10;
constexpr std::size_t RECIPIENT_START_INDEX = 8;

constexpr std::size_t AUTH_PREFIX_LENGTH = 11;

constexpr std::size_t DELIMITER_OFFSET = 2;

namespace ISXCommandHandler
{
CommandHandler::CommandHandler(boost::asio::ssl::context& ssl_context)
    : m_ssl_context(ssl_context), m_data_base(std::make_unique<PgMailDB>("localhost"))
{
    Logger::LogDebug("Entering CommandHandler constructor");
    Logger::LogTrace("Constructor params: ssl_context");

    // Set SSL options
    Logger::LogDebug("Setting SSL options");
    m_ssl_context.set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 |
                              boost::asio::ssl::context::no_sslv3 | boost::asio::ssl::context::no_tlsv1 |
                              boost::asio::ssl::context::no_tlsv1_1);
    Logger::LogProd("SSL options set successfully.");

    // Connect to the database
    try
    {
        ConnectToDatabase();
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception during database connection: " + std::string(e.what()));
        throw;  // Re-throw to ensure proper exception handling at creation time
    }

    Logger::LogDebug("Exiting CommandHandler constructor");
}

CommandHandler::~CommandHandler()
{
    Logger::LogDebug("Entering CommandHandler destructor");

    DisconnectFromDatabase();

    Logger::LogDebug("Exiting CommandHandler destructor");
}

void CommandHandler::ConnectToDatabase() const
{
    Logger::LogDebug("Entering CommandHandler::ConnectToDatabase");

    Logger::LogTrace("CommandHandler::ConnectToDatabase parameter: connection_string=" + m_connection_string);

    try
    {
        m_data_base->Connect(m_connection_string);

        if (!m_data_base->IsConnected())
        {
            const std::string error_message = "Database connection is not established.";
            Logger::LogError("CommandHandler::ConnectToDatabase: " + error_message);
            throw std::runtime_error(error_message);
        }

        Logger::LogDebug("Exiting CommandHandler::ConnectToDatabase successfully");
    }
    catch (const std::exception& e)
    {
        Logger::LogError("CommandHandler::ConnectToDatabase: Exception occurred - " + std::string(e.what()));
        throw;
    }
}

void CommandHandler::DisconnectFromDatabase() const
{
    Logger::LogDebug("Entering CommandHandler::DisconnectFromDatabase");

    if (m_data_base->IsConnected())
    {
        Logger::LogTrace(
            "CommandHandler::DisconnectFromDatabase: "
            "Database is currently connected. Proceeding to disconnect.");

        try
        {
            m_data_base->Disconnect();
            Logger::LogProd(
                "CommandHandler::DisconnectFromDatabase: "
                "Successfully disconnected from the database.");
        }
        catch (const std::exception& e)
        {
            Logger::LogError(
                "CommandHandler::DisconnectFromDatabase: "
                "Error during database disconnection: " +
                std::string(e.what()));
        }
    }
    else
    {
        Logger::LogTrace("CommandHandler::DisconnectFromDatabase: Database was not connected.");
    }

    Logger::LogDebug("Exiting CommandHandler::DisconnectFromDatabase");
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
    else
    {
        try
        {
            socket_wrapper.SendResponseAsync(ToString(SmtpResponseCode::SYNTAX_ERROR)).get();
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
        socket_wrapper.SendResponseAsync(ToString(SmtpResponseCode::OK)).get();
        Logger::LogProd("CommandHandler::HandleEhlo: Successfully sent EHLO response to client.");
    }
    catch (const std::exception& e)
    {
        Logger::LogError("CommandHandler::HandleEhlo: Exception caught while sending EHLO response: " +
                         std::string(e.what()));
    }

    Logger::LogDebug("Exiting CommandHandler::HandleEhlo");
}

void CommandHandler::HandleNoop(SocketWrapper& socket_wrapper)
{
    Logger::LogDebug("Entering CommandHandler::HandleNoop");
    Logger::LogTrace("CommandHandler::HandleNoop parameter: SocketWrapper reference");

    try
    {
        socket_wrapper.SendResponseAsync(ToString(SmtpResponseCode::OK)).get();
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
        socket_wrapper.SendResponseAsync(ToString(SmtpResponseCode::OK)).get();
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
        socket_wrapper.SendResponseAsync(ToString(SmtpResponseCode::HELP_MESSAGE) + " :" + supported_commands).get();
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
        socket_wrapper.SendResponseAsync(ToString(SmtpResponseCode::CLOSING_TRANSMISSION_CHANNEL)).get();
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
        if (!m_data_base->UserExists(sender))
        {
            Logger::LogProd("Sender address doesn't exist: " + sender);
            socket_wrapper
                .SendResponseAsync(ToString(SmtpResponseCode::INVALID_EMAIL_ADDRESS) +
                                   " : sender address doesn't exist.")
                .get();
        }
        else
        {
            m_mail_builder.set_from(sender);
            Logger::LogProd("Sender address set successfully: " + sender);
            socket_wrapper.SendResponseAsync(ToString(SmtpResponseCode::OK)).get();
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
                .SendResponseAsync(ToString(SmtpResponseCode::INVALID_EMAIL_ADDRESS) +
                                   " : recipient address doesn't exist.")
                .get();
            return;
        }
        m_mail_builder.add_to(recipient);
        Logger::LogProd("Recipient address set successfully: " + recipient);
        socket_wrapper.SendResponseAsync(ToString(SmtpResponseCode::OK)).get();
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
        socket_wrapper.SendResponseAsync(ToString(SmtpResponseCode::START_MAIL_INPUT)).get();
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
        std::string buffer = socket_wrapper.ReadFromSocketAsync(MAX_LENGTH).get();
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
/*
void CommandHandler::ForwardToClientMailServer(const std::string& server, int port, const std::string& message) {
    try {
        boost::asio::io_context io_context;
        auto new_socket = std::make_shared<TcpSocket>(io_context);
        SocketWrapper socket_wrapper(new_socket);

        // Connect to the server asynchronously
        auto connect_future = socket_wrapper.Connect(server, port);
        connect_future.get(); // Wait for connection to be established

        std::cout << "Successfully connected to: " << server << " on port " << port << std::endl;

        // Send HELO command
        std::string helo_command = "HELO example.com\r\n";
        auto send_helo_future = socket_wrapper.SendResponseAsync(helo_command);
        send_helo_future.get(); // Wait for the HELO command to be sent

        // Read response from server
        auto read_response_future = socket_wrapper.ReadFromSocketAsync(1024);
        std::string server_response = read_response_future.get(); // Wait and get the response
        std::cout << "Server response: " << server_response << std::endl;

        // Send the actual message
        auto send_message_future = socket_wrapper.SendResponseAsync(message);
        send_message_future.get(); // Wait for the message to be sent

        // Read final response from server
        auto read_final_response_future = socket_wrapper.ReadFromSocketAsync(1024);
        server_response = read_final_response_future.get(); // Wait and get the final response
        std::cout << "Server response after message: " << server_response << std::endl;

    } catch (std::exception& e) {
        std::cerr << "Error connecting to " << server << ": " << e.what() << std::endl;
    }
}*/


void CommandHandler::ForwardToClientMailServer(const std::string& server, int port, const std::string& message) {
    try {
        boost::asio::io_context io_context;
        auto new_socket = std::make_shared<TcpSocket>(io_context);
        SocketWrapper socket_wrapper(new_socket);
        socket_wrapper.Connect(server, port);

        std::cout << "Successfully connected to: " << server << " on port " << port << std::endl;

        std::string helo_command = "HELO example.com\r\n";
        boost::asio::write(*new_socket, boost::asio::buffer(helo_command));

        boost::asio::streambuf response;
        read_until(*new_socket, response, "\r\n");
        std::istream response_stream(&response);
        std::string server_response;
        std::getline(response_stream, server_response);
        std::cout << "Server response: " << server_response << std::endl;

        boost::asio::write(*new_socket, boost::asio::buffer(message));

        read_until(*new_socket, response, "\r\n");
        std::getline(response_stream, server_response);
        std::cout << "Server response after message: " << server_response << std::endl;

    } catch (std::exception& e) {
        std::cerr << "Error connecting to " << server << ": " << e.what() << std::endl;
    }
}
// to only one(the most prioritized MX-record)
void CommandHandler::SendMail(const MailMessage& message) {
    MXResolver mx_resolver;
    std::string domain = mx_resolver.ExtractDomain(message.to.front().get_address());
    std::vector<MXRecord> mx_records = mx_resolver.ResolveMX(domain);

    if (!mx_records.empty()) {
        std::string mail_message = "Subject: " + message.subject + "\r\n\r\n" + message.body;
        ForwardToClientMailServer(mx_records[0].host, 25, mail_message);
    } else {
        std::cerr << "No MX records found for domain: " << domain << std::endl;
    }
}

/*
// to many MX-records
void CommandHandler::SendMail(const MailMessage& message) {
    MXResolver mx_resolver;
    std::set<std::string> domains;

    for (const auto& recipient : message.to) {
        std::string domain = mx_resolver.ExtractDomain(recipient.get_address());
        domains.insert(domain);
    }

    for (const auto& domain : domains) {
        std::vector<MXRecord> mx_records = mx_resolver.ResolveMX(domain);

        if (!mx_records.empty()) {
            std::string mail_message = "Subject: " + message.subject + "\r\n\r\n" + message.body;

            for (const auto& mx_record : mx_records) {
                try {
                    ForwardToClientMailServer(mx_record.host, 25, mail_message);
                } catch (const std::exception& e) {
                    std::cerr << "Failed to send mail to domain " << domain << " via server " << mx_record.host << ": " << e.what() << std::endl;
                }
            }
        } else {
            std::cerr << "No MX records found for domain: " << domain << std::endl;
        }
    }
}*/

void CommandHandler::HandleEndOfData(SocketWrapper& socket_wrapper) {
    Logger::LogDebug("Entering CommandHandler::HandleEndOfData");
    Logger::LogTrace("CommandHandler::ProcessDataMessage parameters: SocketWrapper reference");

    m_in_data = false;
    try {
        MailMessage message = m_mail_builder.Build();
        if (message.from.get_address().empty() || message.to.empty()) {
            auto future_response =
                socket_wrapper.SendResponseAsync(ToString(SmtpResponseCode::REQUIRED_FIELDS_MISSING));
            Logger::LogWarning("Required fields missing in mail message.");
        } else {
            socket_wrapper.SendResponseAsync(ToString(SmtpResponseCode::OK)).get();
            Logger::LogProd("Sent 250 OK response for end of data.");

            SaveMailToDatabase(message);
            Logger::LogProd("Mail message saved successfully.");

            SendMail(message);
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
                m_data_base->InsertEmail(message.from.get_address(), recipient.get_address(), message.subject,
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
            socket_wrapper.SendResponseAsync(ToString(SmtpResponseCode::BAD_SEQUENCE)).get();
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
        socket_wrapper.SendResponseAsync(ToString(SmtpResponseCode::OK)).get();

        Logger::LogDebug("Starting TLS handshake.");
        socket_wrapper.StartTlsAsync(m_ssl_context).get();

        Logger::LogProd("STARTTLS handshake completed successfully.");
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception in CommandHandler::HandleStartTLS: " + std::string(e.what()));
        try
        {
            socket_wrapper.SendResponseAsync(ToString(SmtpResponseCode::TLS_TEMPORARILY_UNAVAILABLE)).get();
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

        // Check if the user exists
        if (!m_data_base->UserExists(username))
        {
            Logger::LogWarning("Authentication failed: user does not exist - " + username);
            socket_wrapper.SendResponseAsync(ToString(SmtpResponseCode::AUTHENTICATION_FAILED)).get();
            return;
        }

        // Attempt to log in with the provided credentials
        try
        {
            m_data_base->Login(username, password);
            Logger::LogProd("User authenticated successfully");
            socket_wrapper.SendResponseAsync(ToString(SmtpResponseCode::AUTH_SUCCESSFUL)).get();
        }
        catch (const MailException& e)
        {
            Logger::LogError("MailException in CommandHandler::HandleAuth: " + std::string(e.what()));
            socket_wrapper.SendResponseAsync(ToString(SmtpResponseCode::AUTH_SUCCESSFUL)).get();
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

        if (m_data_base->UserExists(username))
        {
            Logger::LogWarning("Registration failed: user " + username + " already exists.");
            socket_wrapper.SendResponseAsync(ToString(SmtpResponseCode::USER_ALREADY_EXISTS)).get();
            return;
        }

        m_data_base->SignUp(username, password);
        Logger::LogProd("User registered successfully");
        socket_wrapper.SendResponseAsync(ToString(SmtpResponseCode::REGISTER_SUCCESSFUL)).get();
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
