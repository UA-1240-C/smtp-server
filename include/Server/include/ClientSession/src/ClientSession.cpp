#include "ClientSession.h"

#include "SmtpRequest.h"
#include "SmtpResponse.h"
#include "MailMessage.h"
#include "MailDB/MailException.h"

#include <boost/asio/ssl/context.hpp>

using ISXSmtpResponse::SmtpResponse;
using ISXSmtpResponse::SmtpResponseCode;
using ISXMM::MailMessageBuilder;
using ISXMailDB::MailException;

using ISXMM::MailMessage;

using ISXSmtpRequest::RequestParser;
using ISXSmtpRequest::SmtpCommand;

namespace ISXCState {

ClientSession::ClientSession(TcpSocketPtr socket
                             , boost::asio::ssl::context& ssl_context
                             , std::chrono::seconds timeout_duration)
    : m_current_state(ClientState::CONNECTED)
    , m_socket(socket)
    , m_timeout_duration(timeout_duration)
    , m_data_base(std::make_unique<PgMailDB>("localhost"))
{
    Logger::LogDebug("Entering ClientSession constructor");
    
    ConnectToDataBase();

    m_socket.StartTimeoutTimer(timeout_duration);
    m_ssl_context = &ssl_context;

    Logger::LogDebug("Exiting ClientSession constructor");
};

ClientSession::~ClientSession()
{
    Logger::LogDebug("Entering ClientSession destructor");

    try
    {
        m_data_base->Disconnect();
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception caught while disconnecting from database: " + std::string(e.what()));
    }

    Logger::LogDebug("Exiting ClientSession destructor");
}

void ClientSession::Greet()
{
    Logger::LogProd("Entering ClientSession::Greet");

    try
    {
        m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::SERVER_READY).ToString()).get();
        Logger::LogProd("Successfully sent service ready response to client.");
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception caught while sending service ready response: " + std::string(e.what()));
        throw;
    }

    Logger::LogProd("Exiting ClientSession::Greet");
}

void ClientSession::PollForRequest()
{
    Logger::LogProd("Entering ClientSession::PollForRequest");

    while (true)
    {
        try
        {
            HandleNewRequest();
        }
        catch (const boost::system::system_error& e)
        {
            if (e.code() == boost::asio::error::operation_aborted)
            {
                Logger::LogWarning("Client disconnected.");
                break;
            }
            Logger::LogError("System error in ClientSession::PollForRequest: " + std::string(e.what()));
            throw;
        }
        catch (const std::exception& e)
        {
            Logger::LogError("Exception in ClientSession::PollForRequest: " + std::string(e.what()));
            throw;
        }
    }

    Logger::LogProd("Exiting ClientSession::PollForRequest");
}

void ClientSession::HandleNewRequest() {
    Logger::LogProd("Entering ClientSession::HandleNewRequest");

    if (!m_socket.IsOpen())
    {
        Logger::LogWarning("Client disconnected.");
        throw std::runtime_error("Client disconnected.");
    }

    MailMessageBuilder mail_builder;
    std::string current_line;
    
    Logger::LogProd("Reading data from socket.");
    auto buffer = m_socket.ReadAsync(MAX_LENGTH).get();
    m_socket.ResetTimeoutTimer(m_timeout_duration);
    Logger::LogProd("Received data: " + buffer);

    current_line.append(buffer);
    std::size_t pos;
    while ((pos = current_line.find("\r\n")) != std::string::npos)
    {
        std::string line = current_line.substr(0, pos);
        current_line.erase(0, pos + 2); // +2 to remove the \r\n

        Logger::LogProd("Processing request: " + line);
        try
        {
            ProcessRequest(RequestParser::Parse(line));
        }
        catch (const std::exception& e)
        {
            Logger::LogError("Exception in ClientSession::HandleNewRequest: " + std::string(e.what()));
            m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::SYNTAX_ERROR).ToString()).get();
        }
    }

    Logger::LogProd("Exiting ClientSession::HandleNewRequest");
}

void ClientSession::ProcessRequest(const SmtpRequest& request) {
    Logger::LogDebug("Entering ClientSession::ProcessRequest");
    Logger::LogTrace("ProcessRequest parameter: const SmtpRequest reference: " + request.data);

    if (HandleStaticCommands(request)) return;

    switch (m_current_state)
    {
    case ClientState::CONNECTED:
        HandleConnectedState(request);
        break;
    case ClientState::EHLO_SENT:
        HandleEhloSentState(request);
        break;
    case ClientState::STARTTLS_SENT:
        HandleStartTlsSentState(request);
        break;
    case ClientState::AUTH_SENT:
        HandleAuthSentState(request);
        break;
    case ClientState::MAILFROM_SENT:
        HandleMailFromSentState(request);
        break;
    case ClientState::RCPTTO_SENT:
        HandleRcptToSentState(request);
        break;
    case ClientState::QUIT_SENT:
        HandleQuit(request);
        break;
    default:
        // Unreachable
        Logger::LogError("Invalid state: " + std::to_string(static_cast<int>(m_current_state)));
        m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::BAD_SEQUENCE).ToString()).get();
        throw std::runtime_error("Invalid state.");
    }

    Logger::LogDebug("Exiting ClientSession::ProcessRequest");
}

/********************/
/* Command handlers */
/********************/
bool ClientSession::HandleStaticCommands(const SmtpRequest& request)
{
    Logger::LogDebug("Entering ClientSession::HandleStaticCommands");
    Logger::LogTrace("HandleStaticCommands parameter: const SmtpRequest reference: " + request.data);
    
    bool command_handled = false;

    if (m_current_state == ClientState::CONNECTED
        && request.command == SmtpCommand::EHLO)
    {
        return false;
    }

    if (request.command == SmtpCommand::RSET && 
        (m_current_state == ClientState::MAILFROM_SENT
        || m_current_state == ClientState::RCPTTO_SENT
        || m_current_state == ClientState::DATA_SENT)
    )
    {
        try
        {
            HandleRset(request);
            return true;
        }
        catch (const std::exception& e)
        {
            Logger::LogError("Exception in HandleStaticCommands: " + std::string(e.what()));
        }
    }

    try
    {
        switch (request.command)
        {
        case SmtpCommand::EHLO:
        case SmtpCommand::NOOP:
            m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::OK).ToString()).get();
            command_handled = true;
            break;
        case SmtpCommand::QUIT:
            m_current_state = ClientState::QUIT_SENT;
            HandleQuit(request);
            command_handled = true;
            break;
        case SmtpCommand::HELP:
            m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::HELP_MESSAGE).ToString()).get();
            command_handled = true;
            break;
        default:
            break;
        }
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception in HandleStaticCommands: " + std::string(e.what()));
    }

    Logger::LogDebug("Exiting HandleStaticCommands");
    return command_handled;
}

void ClientSession::HandleRegister(const SmtpRequest& request)
{
    Logger::LogDebug("Entering ClientSession::HandleRegister");
    Logger::LogTrace("ClientSession::HandleRegister parameter: const SmtpRequest reference: " + request.data);

    try
    {
        // Decode the username and password from the AUTH command line
        auto [username, password] = RequestParser::DecodeAndSplitPlain(request.data.substr(REGISTER_PREFIX_LENGTH));
        Logger::LogTrace("Decoded username: " + username);
        Logger::LogTrace("Decoded password: [hidden]");

        // Check if the user exists
        if (m_data_base->UserExists(username))
        {
            Logger::LogWarning("Registration failed: user already exists - " + username);
            m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::USER_ALREADY_EXISTS).ToString()).get();
            return;
        }

        // Attempt to register the user with the provided credentials
        try
        {
            m_data_base->SignUp(username, password);
            Logger::LogProd("User registered successfully");
            m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::REGISTER_SUCCESSFUL).ToString()).get();
        }
        catch (const MailException& e)
        {
            Logger::LogError("MailException in : " + std::string(e.what()));
            m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::REGISTRATION_FAILED).ToString()).get();
        }
    }
    catch (const std::exception& e)
    {
        Logger::LogError(std::string(e.what()));
    }

    Logger::LogDebug("Exiting ClientSession::HandleRegister");
}

void ClientSession::HandleAuth(const SmtpRequest& request)
{
    Logger::LogDebug("Entering ClientSession::HandleAuth");
    Logger::LogTrace("ClientSession::HandleAuth parameter: const SmtpRequest reference: " + request.data);

    try
    {
        // Decode the username and password from the AUTH command line
        auto [username, password] = RequestParser::DecodeAndSplitPlain(request.data.substr(AUTH_PREFIX_LENGTH));
        Logger::LogTrace("Decoded username: " + username);
        Logger::LogTrace("Decoded password: [hidden]");

        // Check if the user exists
        if (!m_data_base->UserExists(username))
        {
            Logger::LogWarning("Authentication failed: user does not exist - " + username);
            m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::AUTHENTICATION_FAILED).ToString()).get();
            return;
        }

        // Attempt to log in with the provided credentials
        try
        {
            m_data_base->Login(username, password);
            Logger::LogProd("User authenticated successfully");
            m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::AUTH_SUCCESSFUL).ToString()).get();
        }
        catch (const MailException& e)
        {
            Logger::LogError("MailException in : " + std::string(e.what()));
            m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::AUTH_SUCCESSFUL).ToString()).get();
        }
    }
    catch (const std::exception& e)
    {
        Logger::LogError(std::string(e.what()));
    }

    Logger::LogDebug("Exiting ClientSession::HandleAuth");
}

void ClientSession::HadleStartTls(const SmtpRequest& request)
{
    Logger::LogDebug("Entering ClientSession::HandleEhloSentState");
    Logger::LogTrace("ClientSession::HandleEhloSentState parameter: const SmtpRequest reference: " + request.data);

    if (m_socket.IsTls())
    {
        Logger::LogWarning("STARTTLS command received but already in TLS mode.");
        try
        {
           m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::BAD_SEQUENCE).ToString()).get();
        }
        catch (const std::exception& e)
        {
            Logger::LogError("Exception in HandleEhloSentState: " + std::string(e.what()));
        }

        Logger::LogDebug("Exiting ClientSession::HandleEhloSentState");
        return;
    }

    try
    {
        Logger::LogProd("Sending response to indicate readiness to start TLS.");
        m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::OK).ToString()).get();

        Logger::LogDebug(m_socket.IsTls() ? "true" : "false");

        Logger::LogDebug("Starting TLS handshake.");
        m_socket.PerformTlsHandshake(boost::asio::ssl::stream_base::server, *m_ssl_context).get();

        Logger::LogProd("STARTTLS handshake completed successfully.");
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception in HandleEhloSentState: " + std::string(e.what()));
        try
        {
            m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::TLS_TEMPORARILY_UNAVAILABLE).ToString()).get();
        }
        catch (const std::exception& send_e)
        {
            Logger::LogError(std::string(send_e.what()));
        }
    }
}

void ClientSession::HandleMailFrom(const SmtpRequest& request)
{
    Logger::LogDebug("Entering ClientSession::HandleMailFrom");
    Logger::LogTrace("ClientSession::HandleMailFrom parameter: const SmtpRequest reference: " + request.data);

    std::string sender;

    try
    {
        sender = request.data.substr(10);
        sender = RequestParser::ExtractUsername(sender);

        Logger::LogDebug("Parsed sender: " + sender);
    }
    catch (const std::exception& e)
    {
        Logger::LogError(std::string(e.what()));
    }

    try
    {
        if (!m_data_base->UserExists(sender))
        {
            Logger::LogProd("Sender address doesn't exist: " + sender);
            m_socket
                .WriteAsync(SmtpResponse(SmtpResponseCode::INVALID_EMAIL_ADDRESS).ToString() +
                                   " : sender address doesn't exist.")
                .get();
        }
        else
        {
            m_mail_builder.set_from(sender);
            Logger::LogProd("Sender address set successfully: " + sender);
            m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::OK).ToString()).get();
        }
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception in ClientSession::HandleMailFrom: " + std::string(e.what()));
    }

    Logger::LogDebug("Exiting ClientSession::HandleMailFrom");
}

void ClientSession::HandleRcptTo(const SmtpRequest& request)
{
    Logger::LogDebug("Entering ClientSession::HandleRcptTo");
    Logger::LogTrace("ClientSession::HandleRcptTo parameter: const SmtpRequest reference: " + request.data);

    std::string recipient = request.data.substr(RECIPIENT_START_INDEX);
    Logger::LogTrace("Parsed recipient: " + recipient);

    recipient = RequestParser::ExtractUsername(recipient);

    try
    {
        if (!m_data_base->UserExists(recipient))
        {
            Logger::LogProd("Recipient address does not exist: " + recipient);
            m_socket
                .WriteAsync(SmtpResponse(SmtpResponseCode::INVALID_EMAIL_ADDRESS).ToString() +
                                   " : recipient address doesn't exist.")
                .get();
            return;
        }
        m_mail_builder.add_to(recipient);
        Logger::LogProd("Recipient address set successfully: " + recipient);
        m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::OK).ToString()).get();
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception in ClientSession::HandleRcptTo: " +
                         std::string(e.what()));
    }
    Logger::LogDebug("Exiting ClientSession::HandleRcptTo");
}

void ClientSession::HandleData(const SmtpRequest& request)
{
    Logger::LogDebug("Entering ClientSession::HandleData");
    Logger::LogTrace("ClientSession::HandleData parameter: const SmtpRequest reference: " + request.data);

    try
    {
        m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::START_MAIL_INPUT).ToString()).get();
        Logger::LogProd("Sent response for DATA command, waiting for data.");

        try
        {
            std::string buffer = ReadDataUntilEOM();
            m_socket.ResetTimeoutTimer(m_timeout_duration);
            Logger::LogProd("Received data: " + buffer);
            
            BuildMessageData(buffer);
            MailMessage message = m_mail_builder.Build();
            
            if (message.from.get_address().empty()
                || message.body.empty()
                || message.to.empty()
                || message.subject.empty())
            {
                Logger::LogError("Subject or body is empty.");
                m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::SYNTAX_ERROR).ToString()).get();
                return;
            }
            else
            {
                SaveMessageToDataBase(message);
                m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::OK).ToString()).get();
            }

        }
        catch (const boost::system::system_error& e)
        {
            // Handle specific boost system errors (e.g., client disconnection)
            Logger::LogError("System error in ClientSession::HandleData(Client disconnected): " + std::string(e.what()));
        }
        catch (const std::exception& e)
        {
            // Handle other exceptions
            Logger::LogError("Exception in ClientSession::HandleData: " + std::string(e.what()));
            throw;
        }

        Logger::LogProd("Data handling complete.");
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception in ClientSession::HandleData: " + std::string(e.what()));
    }

    Logger::LogDebug("Exiting ClientSession::HandleData");
}

void ClientSession::HandleRset(const SmtpRequest& request)
{
    Logger::LogDebug("Entering ClientSession::HandleRset");
    Logger::LogTrace("ClientSession::HandleRset parameter: const SmtpRequest reference: " + request.data);

    try
    {
        m_mail_builder = MailMessageBuilder();
        m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::OK).ToString()).get();
        m_current_state = ClientState::AUTH_SENT;
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception in ClientSession::HandleRset: " + std::string(e.what()));
    }

    Logger::LogDebug("Exiting ClientSession::HandleRset");
}

void ClientSession::HandleQuit(const SmtpRequest& request)
{
    Logger::LogDebug("Entering ClientSession::HandleQuit");
    Logger::LogTrace("ClientSession::HandleQuit parameter: const SmtpRequest reference: " + request.data);

    try
    {
        m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::CLOSING_TRANSMISSION_CHANNEL).ToString()).get();
        Logger::LogProd("Sent QUIT response to client.");
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception in ClientSession::HandleQuit: " + std::string(e.what()));
        return;
    }

    m_socket.Close();
    Logger::LogProd("Connection closed by client.");
    Logger::LogDebug("Exiting ClientSession::HandleQuit");

    throw std::runtime_error("Client disconnected");
}

/******************/
/* State handlers */
/******************/
void ClientSession::HandleConnectedState(const SmtpRequest& request)
{
    Logger::LogDebug("Entering ClientSession::HandleConnectedState");
    Logger::LogTrace("HandleConnectedState::HandleEhlo parameter: const SmtRequest reference");
    
    try
    {
        if (request.command == SmtpCommand::EHLO)
        {
            m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::OK).ToString()).get();
            m_current_state = ClientState::EHLO_SENT;
        }
        else 
        {
            Logger::LogError(std::to_string(static_cast<int>(m_current_state)));
            m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::BAD_SEQUENCE).ToString()).get();
        }
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception in ClientSession::HandleConnectedState: " + std::string(e.what()));
    }

    Logger::LogDebug("Exiting ClientSession::HandleConnectedState");
}

void ClientSession::HandleEhloSentState(const SmtpRequest& request)
{
    Logger::LogDebug("Entering ClientSession::HandleEhloSentState");
    Logger::LogTrace("ClientSession::HandleEhloSentState parameter: const SmtpRequest reference: " + request.data);

    if (request.command == SmtpCommand::STARTTLS)
    {
        HadleStartTls(request);
        m_current_state = ClientState::STARTTLS_SENT;
    }
    else
    {
        Logger::LogError(std::to_string(static_cast<int>(m_current_state)));
        m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::BAD_SEQUENCE).ToString()).get();
    }

    Logger::LogDebug("Exiting HandleEhloSentState");
}

void ClientSession::HandleStartTlsSentState(const SmtpRequest& request)
{
    Logger::LogDebug("Entering ClientSession::HandleStartTlsSentState");
    Logger::LogTrace("ClientSession::HandleStartTlsSentState parameter: const SmtpRequest reference: " + request.data);

    if (request.command == SmtpCommand::AUTH)
    {
        HandleAuth(request);
        m_current_state = ClientState::AUTH_SENT;
    }
    else if (request.command == SmtpCommand::REGISTER)
    {
        HandleRegister(request);
        m_current_state = ClientState::AUTH_SENT;
    }    else
    {
        Logger::LogError(std::to_string(static_cast<int>(m_current_state)));
        m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::BAD_SEQUENCE).ToString()).get();
    }

    Logger::LogDebug("Exiting ClientSession::HandleStartTlsSentState");
}

void ClientSession::HandleAuthSentState(const SmtpRequest& request)
{
    Logger::LogDebug("Entering ClientSession::HandleAuthSentState");
    Logger::LogTrace("ClientSession::HandleAuthSentState parameter: const SmtpRequest reference: " + request.data);

    if (request.command == SmtpCommand::MAILFROM)
    {
        HandleMailFrom(request);
        m_current_state = ClientState::MAILFROM_SENT;
    }
    else
    {
        Logger::LogError(std::to_string(static_cast<int>(m_current_state)));
        m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::BAD_SEQUENCE).ToString()).get();
    }
}

void ClientSession::HandleMailFromSentState(const SmtpRequest& request)
{
    Logger::LogDebug("Entering ClientSession::HandleMailFromSentState");
    Logger::LogTrace("ClientSession::HandleMailFromSentState parameter: const SmtpRequest reference: " + request.data);

    if (request.command == SmtpCommand::RCPTTO)
    {
        HandleRcptTo(request);
        m_current_state = ClientState::RCPTTO_SENT;
    }
    else
    {
        Logger::LogError(std::to_string(static_cast<int>(m_current_state)));
        m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::BAD_SEQUENCE).ToString()).get();
    }

    Logger::LogDebug("Exiting ClientSession::HandleMailFromSentState");
}

void ClientSession::HandleRcptToSentState(const SmtpRequest& request) 
{
    Logger::LogDebug("Entering ClientSession::HandleRcptToSentState");
    Logger::LogTrace("ClientSession::HandleRcptToSentState parameter: const SmtpRequest reference: " + request.data);

    if (request.command == SmtpCommand::DATA)
    {
        HandleData(request);
        m_current_state = ClientState::AUTH_SENT;
    }
    else
    {
        Logger::LogError(std::to_string(static_cast<int>(m_current_state)));
        m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::BAD_SEQUENCE).ToString()).get();
    }

    Logger::LogDebug("Exiting ClientSession::HandleRcptToSentState");
}

/*********************/
/* Utility functions */
/*********************/
std::string ClientSession::ReadDataUntilEOM()
{
    Logger::LogDebug("Entering ClientSession::ReadDataUntilEOM");

    std::string buffer;
    std::string data;

    try
    {
        do
        {
            buffer = m_socket.ReadAsync(MAX_LENGTH).get();
            if (buffer.empty())
            {
                Logger::LogWarning("Client disconnected.");
                throw std::runtime_error("Client disconnected.");
            }
            m_socket.ResetTimeoutTimer(m_timeout_duration);
            data += buffer;
        } while (buffer.find("\r\n.\r\n") == std::string::npos);

        Logger::LogProd("Data received: " + data);
    }
    catch (const boost::system::system_error& e)
    {
        Logger::LogError("System error in ClientSession::ReadDataUntilEOM: " + std::string(e.what()));
        throw;
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception in ClientSession::ReadDataUntilEOM: " + std::string(e.what()));
        throw;
    }

    Logger::LogDebug("Exiting ClientSession::ReadDataUntilEOM");
    return data;
}

void ClientSession::ConnectToDataBase()
{
    Logger::LogDebug("Entering ClientSession::ConnectToDataBase");

    try
    {
        m_data_base->Connect(m_connection_string);

        if (!m_data_base->IsConnected())
        {
            const std::string error_message = "Database connection is not established.";
            Logger::LogError("ClientSession::ConnectToDatabase: " + error_message);
            throw std::runtime_error(error_message);
        }

        Logger::LogDebug("Exiting ClientSession::ConnectToDatabase successfully");
    }
    catch (const std::exception& e)
    {
        Logger::LogError("ClientSession::ConnectToDatabase: Exception occurred - " + std::string(e.what()));
        throw;
    }
    Logger::LogDebug("Exiting ClientSession::ConnectToDataBase");
}

void ClientSession::BuildMessageData(const std::string& data)
{
    Logger::LogDebug("Entering ClientSession::BuildMessage");
    Logger::LogTrace("ClientSession::BuildMessage parameter: const std::string reference: " + data);

    try
    {
        std::string subject = RequestParser::ExtractSubject(data);
        std::string body = RequestParser::ExtractBody(data);

        m_mail_builder.set_subject(subject);
        m_mail_builder.set_body(body);
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception in ClientSession::BuildMessage: " + std::string(e.what()));
        throw;
    }

    Logger::LogDebug("Exiting ClientSession::BuildMessage");
}

void ClientSession::SaveMessageToDataBase(MailMessage& message)
{
    Logger::LogDebug("Entering ClientSession::SaveMessageToDataBase");

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
                    "Exception in ClientSession::SaveMessageToDataBase: " + std::string(e.what()));
            }
        }
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception in ClientSession::SaveMessageToDataBase: " + std::string(e.what()));
        throw;
    }


    Logger::LogDebug("Exiting ClientSession::SaveMessageToDataBase");
}

} // namespace ISXCState
