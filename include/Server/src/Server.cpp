#include "Server.h"

#include "ClientSession.h"
using ISXCState::ClientSession;

namespace ISXSS
{
SmtpServer::SmtpServer(boost::asio::io_context& io_context, boost::asio::ssl::context& ssl_context)
    : m_initializer(io_context, ssl_context) 
{
    Logger::LogDebug("Entering SmtpServer constructor");
    Logger::LogTrace("Constructor params: io_context, ssl_context");
    Logger::LogDebug("Exiting SmtpServer constructor");
}

void SmtpServer::Start()
{
    Logger::LogDebug("Entering Start");

    Accept();

    Logger::LogDebug("Exiting Start");
}

void SmtpServer::Accept()
{
    Logger::LogDebug("Entering SmtpServer::Accept");
    auto new_socket = std::make_shared<TcpSocket>(m_initializer.get_io_context());

    Logger::LogProd("Ready to accept new connections.");

    auto& acceptor = m_initializer.get_acceptor();
    acceptor.async_accept(*new_socket,
        [this, new_socket](const boost::system::error_code& error)
        {
            if (!error)
            {
                Logger::LogProd("Accepted new connection.");
                m_initializer.get_thread_pool().EnqueueDetach(
                    [this, new_socket] {
                        try{
                            unique_ptr<ClientSession> client_session;
                            client_session = unique_ptr<ClientSession>
                                (new ClientSession(new_socket
                                                   , m_initializer.get_ssl_context()
                                                   , m_initializer.get_timeout_seconds()));

                            client_session->Greet();
                            client_session->PollForRequest();
                        }
                        catch (const std::exception& e)
                        {
                            Logger::LogError("Error in SmtpServer::Accept: " + std::string(e.what()));
                        }
                    }
                );
            }
            else
            {
                Logger::LogError("Boost error in SmtpServer::Accept" + error.what());
            }
            Accept();
        }
    );

    Logger::LogDebug("Exiting Accept");
}

void SmtpServer::ResetTimeoutTimer(SocketWrapper& socket_wrapper)
{
    Logger::LogDebug("Entering SmtpServer::ResetTimeoutTimer");
    Logger::LogTrace("SmtpServer::ResetTimeoutTimer params: SocketWrapper reference");

    auto timeout_timer = std::make_shared<boost::asio::steady_timer>(m_initializer.get_io_context());
    socket_wrapper.set_timeout_timer(timeout_timer);
    socket_wrapper.StartTimeoutTimer(m_initializer.get_timeout_seconds());
    Logger::LogDebug("Exiting SmtpServer::ResetTimeoutTimer");
}

void SmtpServer::HandleClient(SocketWrapper socket_wrapper)
{
    Logger::LogDebug("Entering SmtpServer::HandleClient");
    Logger::LogTrace("SmtpServer::HandleClient parameters: SocketWrapper");

    socket_wrapper.WriteAsync(ToString(SmtpResponseCode::SERVER_READY)).get();

    try
    {
        CommandHandler command_handler(m_initializer.get_io_context(), m_initializer.get_ssl_context(), m_initializer.get_database_manager());
        MailMessageBuilder mail_builder;
        std::string current_line;

        while (true)
        {
            if (!socket_wrapper.IsOpen()) break;

            Logger::LogProd("Reading data from socket.");

            auto future_data = socket_wrapper.ReadAsync(MAX_LENGTH);

            try
            {
                std::string buffer = future_data.get();

                Logger::LogProd("Received data: " + buffer);

                current_line.append(buffer);
                std::size_t pos;
                while ((pos = current_line.find("\r\n")) != std::string::npos)
                {
                    std::string line = current_line.substr(0, pos);
                    current_line.erase(0, pos + DELIMITER_OFFSET);

                    Logger::LogProd("Processing line: " + line);

                    command_handler.ProcessLine(line, socket_wrapper);
                }

                socket_wrapper.CancelTimeoutTimer();
                ResetTimeoutTimer(socket_wrapper);
            }
            catch (const boost::system::system_error& e)
            {
                if (e.code() == boost::asio::error::operation_aborted)
                {
                    Logger::LogWarning("Client disconnected.");
                    break;
                }
                Logger::LogError("Boost exception in SmtpServer::HandleClient while reading from socket" +
                                 std::string(e.what()));
                throw;
            }
            catch (const std::exception& e)
            {
                Logger::LogError("Exception in SmtpServer::HandleClient while reading from socket" +
                                 std::string(e.what()));
                throw;
            }
        }
    }
    catch (std::exception& e)
    {
        Logger::LogError("SmtpServer::HandleClient: Exception occurred - " + std::string(e.what()));
    }

    Logger::LogDebug("Exiting SmtpServer::HandleClient");
}
}  // namespace ISXSS
