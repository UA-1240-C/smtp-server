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
                                                   , m_initializer.get_database_manager()
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
}  // namespace ISXSS
