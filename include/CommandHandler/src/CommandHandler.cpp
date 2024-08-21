#include "CommandHandler.h"

const std::size_t MAILING_LIST_PREFIX_LENGTH = 5;

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
    catch (const MailException& e)
    {
        Logger::LogError("MailException during database connection: " + std::string(e.what()));
        throw;  // Re-throw to ensure proper exception handling at creation time
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

    // Disconnect from the database
    try
    {
        DisconnectFromDatabase();
    }
    catch (const MailException& e)
    {
        Logger::LogError("MailException during database disconnect: " + std::string(e.what()));
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception during database disconnect: " + std::string(e.what()));
    }

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
        socket_wrapper.SendResponseAsync("500 Syntax error, command unrecognized\r\n").get();
        Logger::LogWarning("A client sent a unrecognized command to CommandHandler::ProcessLine");
    }

    Logger::LogDebug("Exiting CommandHandler::ProcessLine");
}

void CommandHandler::HandleEhlo(SocketWrapper& socket_wrapper)
{
    Logger::LogDebug("Entering CommandHandler::HandleEhlo");
    Logger::LogTrace("CommandHandler::HandleEhlo parameter: SocketWrapper reference");

    try
    {
        socket_wrapper.SendResponseAsync("250 Hello\r\n").get();
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
        socket_wrapper.SendResponseAsync("250 OK\r\n").get();
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
        socket_wrapper.SendResponseAsync("250 OK\r\n").get();
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
        socket_wrapper.SendResponseAsync("214 The following commands are recognized: " + supported_commands).get();
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
        socket_wrapper.SendResponseAsync("221 OK\r\n").get();
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
            socket_wrapper.SendResponseAsync("550 Sender address does not exist\r\n").get();
        }
        else
        {
            m_mail_builder.SetFrom(sender);
            Logger::LogProd("Sender address set successfully: " + sender);
            socket_wrapper.SendResponseAsync("250 OK\r\n").get();
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
            socket_wrapper.SendResponseAsync("550 Recipient address does not exist\r\n").get();
            return;
        }
        m_mail_builder.AddTo(recipient);
        Logger::LogProd("Recipient address set successfully: " + recipient);
        socket_wrapper.SendResponseAsync("250 OK\r\n").get();
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception in CommandHandler::HandleRcptTo while processing recipient  : " +
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
        // Send response indicating readiness to receive data
        socket_wrapper.SendResponseAsync("354 End data with <CR><LF>.<CR><LF>\r\n").get();
        Logger::LogProd("Sent response for DATA command, waiting for data.");

        m_in_data = true;

        while (m_in_data)
        {
            std::string data_message;
            ReadData(socket_wrapper, data_message);
            ProcessDataMessage(socket_wrapper, data_message);
        }

        Logger::LogProd("Data handling complete.");
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception in HandleData: " + std::string(e.what()));
        throw;
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
        auto future_data = socket_wrapper.ReadFromSocketAsync(1024);
        std::string buffer = future_data.get();
        data_message.append(buffer);

        // Log received data (may be very verbose)
        Logger::LogTrace("Received data: " + buffer);
    }
    catch (const boost::system::system_error& e)
    {
        // Handle specific boost system errors (e.g., client disconnection)
        Logger::LogError("System error during data read(Client disconnected): " + std::string(e.what()));
        m_in_data = false;
    }
    catch (const std::exception& e)
    {
        // Handle other exceptions
        Logger::LogError("Exception in ReadData: " + std::string(e.what()));
        throw;
    }

    Logger::LogDebug("Exiting CommandHandler::ReadData");
}

void CommandHandler::ProcessDataMessage(SocketWrapper& socket_wrapper, std::string& data_message)
{
    Logger::LogDebug("Exiting CommandHandler::ProcessDataMessage");
    Logger::LogTrace(
        "CommandHandler::ProcessDataMessage parameters: "
        "SocketWrapper reference, std::string reference " +
        data_message);

    std::size_t last_pos{};

    while ((last_pos = data_message.find("\r\n", last_pos)) != std::string::npos)
    {
        std::string line = data_message.substr(0, last_pos);
        last_pos += DELIMITER_OFFSET;

        if (line == ".")
        {
            Logger::LogProd("End-of-data sequence detected, exiting data read loop.");
            HandleEndOfData(socket_wrapper);
            break;
        }

        m_mail_builder.SetBody(line);
        Logger::LogProd("Mail body set.");
    }
    Logger::LogDebug("Exiting CommandHandler::ProcessDataMessage");
}


void CommandHandler::HandleEndOfData(SocketWrapper& socket_wrapper)
{
    Logger::LogDebug("Entering CommandHandler::HandleEndOfData");
    Logger::LogTrace("CommandHandler::ProcessDataMessage parameters: SocketWrapper reference");

    m_in_data = false;
    try
    {
        MailMessage message = m_mail_builder.Build();
        if (message.from.get_address().empty() || message.to.empty())
        {
            auto future_response = socket_wrapper.SendResponseAsync("550 Required fields missing\r\n");
            Logger::LogWarning("Required fields missing in mail message.");
        }
        else
        {
            socket_wrapper.SendResponseAsync("250 OK\r\n").get();
            Logger::LogProd("Sent 250 OK response for end of data.");

            SaveMailToDatabase(message);
            Logger::LogProd("Mail message saved successfully.");
        }
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception in CommandHandler::HandleEndOfData while saving mail to DB: " +
                         std::string(e.what()));
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
        Logger::LogError("Exception in CommandHandler::SaveMailToDatabase while saving mail: " + std::string(e.what()));
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
            socket_wrapper.SendResponseAsync("503 Already in TLS mode.\r\n").get();
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
        socket_wrapper.SendResponseAsync("220 Ready to start TLS\r\n").get();

        Logger::LogDebug("Starting TLS handshake.");
        socket_wrapper.StartTlsAsync(m_ssl_context).get();

        Logger::LogProd("STARTTLS handshake completed successfully.");
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception during STARTTLS handshake: " + std::string(e.what()));
        try
        {
            socket_wrapper.SendResponseAsync("530 STARTTLS handshake failed\r\n").get();
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
            socket_wrapper.SendResponseAsync("535 Authentication failed\r\n").get();
            return;
        }

        // Attempt to log in with the provided credentials
        try
        {
            m_data_base->Login(username, password);
            Logger::LogProd("User authenticated successfully");
            socket_wrapper.SendResponseAsync("235 Authentication successful\r\n").get();
        }
        catch (const MailException& e)
        {
            Logger::LogError("MailException in CommandHandler::HandleAuth: " + std::string(e.what()));
            socket_wrapper.SendResponseAsync("535 Authentication failed\r\n").get();
        }
    }
    catch (const std::runtime_error& e)
    {
        Logger::LogError("Runtime error in CommandHandler::HandleAuth: " + std::string(e.what()));
    }
    catch (const MailException& e)
    {
        Logger::LogError("MailException in CommandHandler::HandleAuth: " + std::string(e.what()));
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
            Logger::LogWarning("Authentication failed: user does not exist - " + username);
            socket_wrapper.SendResponseAsync("550 User already exists\r\n").get();
            return;
        }

        m_data_base->SignUp(username, password);
        Logger::LogProd("User registered successfully");
        socket_wrapper.SendResponseAsync("250 User registered successfully\r\n").get();
    }
    catch (const std::runtime_error& e)
    {
        Logger::LogError("Runtime error in CommandHandler::HandleRegister: " + std::string(e.what()));
    }
    catch (const MailException& e)
    {
        Logger::LogError("MailException in CommandHandler::HandleRegister: " + std::string(e.what()));
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
    Logger::LogTrace("First null byte at position: " + std::to_string(first_null));

    // Find the second null byte
    const size_t second_null = decoded_data.find('\0', first_null + 1);
    if (second_null == std::string::npos)
    {
        Logger::LogError("Invalid PLAIN format: Missing second null byte.");
        throw std::runtime_error("Invalid PLAIN format: Missing second null byte.");
    }
    Logger::LogTrace("Second null byte at position: " + std::to_string(second_null));

    // Extract username and password
    std::string username = decoded_data.substr(first_null + 1, second_null - first_null - 1);
    std::string password = decoded_data.substr(second_null + 1);

    // Log the extracted username and password (password should be handled securely in real applications)
    Logger::LogTrace("Extracted username: " + username);
    Logger::LogTrace("Extracted password: [hidden]");

    Logger::LogDebug("Exiting CommandHandler::DecodeAndSplitPlain");

    return {username, password};
}
}  // namespace ISXCommandHandler