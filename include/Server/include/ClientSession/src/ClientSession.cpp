#include "ClientSession.h"
#include "SmtpRequest.h"
#include "SocketWrapper.h"
#include "SmtpResponse.h"
#include "MailMessageBuilder.h"
#include <boost/asio/ssl/context.hpp>

using ISXSmtpResponse::SmtpResponse;
using ISXSmtpResponse::SmtpResponseCode;
using ISXMM::MailMessageBuilder;

using ISXSmtpRequest::RequestParser;
using ISXSmtpRequest::SmtpCommand;

namespace ISXCState {

ClientSession::ClientSession(TcpSocketPtr socket
                             , boost::asio::ssl::context& ssl_context
                             , std::chrono::seconds timeout_duration)
    : m_current_state(ClientState::CONNECTED)
    , m_socket(socket)
    , m_timeout_duration(timeout_duration)
{
    Logger::LogDebug("Entering ClientSession constructor");
    Logger::LogTrace("Constructor params: TcpSocketPtr");
    m_socket.StartTimeoutTimer(timeout_duration);
    m_ssl_context = &ssl_context;
    Logger::LogDebug("Exiting ClientSession constructor");
};

void ClientSession::Greet() {
    Logger::LogProd("Entering ClientSession::Greet");

    try {
        m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::SERVER_READY).ToString()).get();
        Logger::LogProd("Successfully sent service ready response to client.");
    } catch (const std::exception& e) {
        Logger::LogError("Exception caught while sending service ready response: " + std::string(e.what()));
        throw;
    }

    Logger::LogProd("Exiting ClientSession::Greet");
}

void ClientSession::PollForRequest() {
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
            Logger::LogError(std::string(e.what()));
            throw;
        }
        catch (const std::exception& e)
        {
            Logger::LogError(std::string(e.what()));
            throw;
        }
    }

    Logger::LogProd("Exiting ClientSession::PollForRequest");
}

void ClientSession::HandleNewRequest() {
    Logger::LogProd("Entering ClientSession::HandleNewRequest");

    if (!m_socket.IsOpen()) {
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
            Logger::LogError(std::string(e.what()));
            m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::SYNTAX_ERROR).ToString()).get();
        }
    }

    Logger::LogProd("Exiting ClientSession::HandleNewRequest");
}

void ClientSession::ProcessRequest(const SmtpRequest& request) {
    Logger::LogDebug("Entering CommandHandler::ProcessRequest");
    Logger::LogTrace("ProcessRequest parameter: SmtpRequest reference");

    switch (m_current_state)
    {
    case ClientState::CONNECTED:
        HandleConnectedState(request);
        break;
    case ClientState::EHLO_SENT:
        HandleEhloSentState(request);
        break;
    default:
        break;
    }

    Logger::LogDebug("Exiting ProcessRequest");
}

void ClientSession::HandleStaticCommands(const SmtpRequest& request) {
    Logger::LogDebug("Entering HandleStaticCommands::HandleStaticCommands");
    Logger::LogTrace("HandleStaticCommands parameter: SmtpRequest reference");

    try{
        switch (request.command)
        {
        case SmtpCommand::EHLO:
            m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::OK).ToString()).get();
        case SmtpCommand::QUIT:
            m_current_state = ClientState::QUIT_SENT;
            m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::CLOSING_TRANSMISSION_CHANNEL).ToString()).get();
            break;
        case SmtpCommand::HELP:
            m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::HELP_MESSAGE).ToString()).get();
        case SmtpCommand::NOOP:
            m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::OK).ToString()).get();
        default:
            Logger::LogError(std::to_string(static_cast<int>(m_current_state)));
            m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::BAD_SEQUENCE).ToString()).get();
            break;
        }
    }
    catch (const std::exception& e)
    {
        Logger::LogError(std::string(e.what()));
    }

    Logger::LogDebug("Exiting HandleStaticCommands");
}

void ClientSession::HandleConnectedState(const SmtpRequest& request) {
    Logger::LogDebug("Entering CommandHandler::HandleEhlo");
    Logger::LogTrace("HandleConnectedState::HandleEhlo parameter: SocketWrapper reference");
    
    try{
        if (request.command == SmtpCommand::EHLO) {
            m_current_state = ClientState::EHLO_SENT;
            m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::OK).ToString()).get();
        } else {
            Logger::LogError(std::to_string(static_cast<int>(m_current_state)));
            m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::BAD_SEQUENCE).ToString()).get();
        }
    }
    catch (const std::exception& e)
    {
        Logger::LogError(std::string(e.what()));
    }

    Logger::LogDebug("Exiting CommandHandler::HandleEhlo");
}

void ClientSession::HandleEhloSentState(const SmtpRequest& request) {
    Logger::LogDebug("Entering HandleEhloSentState::HandleEhlo");
    Logger::LogTrace("HandleEhloSentState::HandleEhlo parameter: SmtRequest const reference");

    if (request.command == SmtpCommand::STARTTLS) {
        if (m_socket.IsTls())
        {
            Logger::LogWarning("STARTTLS command received but already in TLS mode.");
            try
            {
               m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::BAD_SEQUENCE).ToString()).get();
            }
            catch (const std::exception& e)
            {
                Logger::LogError(std::string(e.what()));
            }

            Logger::LogDebug("Exiting HandleEhloSentState");
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
            Logger::LogError(std::string(e.what()));
            try
            {
                m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::TLS_TEMPORARILY_UNAVAILABLE).ToString()).get();
            }
            catch (const std::exception& send_e)
            {
                Logger::LogError(std::string(send_e.what()));
            }
        }
    } else {
        Logger::LogError(std::to_string(static_cast<int>(m_current_state)));
        m_socket.WriteAsync(SmtpResponse(SmtpResponseCode::BAD_SEQUENCE).ToString()).get();
    }

    Logger::LogDebug("Exiting HandleEhloSentState");
}

} // namespace ISXCState
