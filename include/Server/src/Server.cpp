#include "Server.h"

namespace ISXSS
{
SmtpServer::SmtpServer(boost::asio::io_context& io_context,
       boost::asio::ssl::context& ssl_context,
       const unsigned short port,
       ThreadPool<>& thread_pool)
    : m_thread_pool(thread_pool)
    , m_io_context(io_context)
    , m_ssl_context(ssl_context)
    , m_command_handler(ssl_context)
    , m_acceptor(std::make_unique<tcp::acceptor>(
      io_context, tcp::endpoint(tcp::v4(), port)))
{
    std::cout << "SmtpServer initialized and listening on port " << port
        << std::endl;
}

void SmtpServer::Start() { Accept(); }

void SmtpServer::Accept() {
    auto new_socket = std::make_shared<TcpSocket>(m_io_context);

    m_acceptor->async_accept(
        *new_socket,
        [this, new_socket](const boost::system::error_code& error) {
            if (!error) {
                std::cout << "Accepted new connection." << std::endl;
                // Dispatch the client handling to the thread pool
                m_thread_pool.EnqueueDetach([this, new_socket]() {
                    HandleClient(SocketWrapper(new_socket));
                });
            } else {
                ErrorHandler::HandleBoostError("Accept", error);
            }
            Accept();
        });
}

void SmtpServer::HandleClient(SocketWrapper socket_wrapper) {
    try {
        MailMessageBuilder mail_builder;
        std::string current_line;
        std::size_t length = 1024;

        while (true) {
            auto future_data = socket_wrapper.ReadFromSocketAsync(length);

            try {
                std::string buffer = future_data.get();
                current_line.append(buffer);
                std::size_t pos;

                while ((pos = current_line.find("\r\n")) != std::string::npos) {
                    std::string line = current_line.substr(0, pos);
                    current_line.erase(0, pos + 2);
                    m_command_handler.ProcessLine(line, socket_wrapper);
                }
            } catch (const std::exception& e) {
                ErrorHandler::HandleException("Read from socket", e);
                if (dynamic_cast<const boost::system::system_error*>(&e)) {
                    std::cout << "Client disconnected." << std::endl;
                    break;
                }
                throw;
            }
        }
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}
}