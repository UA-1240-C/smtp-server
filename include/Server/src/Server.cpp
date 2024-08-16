#include "Server.h"

constexpr std::size_t MAX_LENGTH = 1024;

namespace ISXSS
{
// Constructor for the SmtpServer class
SmtpServer::SmtpServer(boost::asio::io_context& io_context, boost::asio::ssl::context& ssl_context)
    : m_io_context(io_context),
      m_ssl_context(ssl_context),
      m_command_handler(ssl_context),
      m_timeout_timer(io_context),
      m_thread_pool([&] {
          Config config("../config.txt");
          Config::ThreadPool thread_pool_config = config.get_thread_pool();
          size_t max_threads = thread_pool_config.max_working_threads > std::thread::hardware_concurrency()
                                   ? std::thread::hardware_concurrency()
                                   : thread_pool_config.max_working_threads;
          return ThreadPool<>(max_threads);
      }()) {
    Config config("../config.txt");
    Config::Server server = config.get_server();
    m_port = server.listener_port;
    m_server_display_name = server.server_display_name;
    m_server_name = server.server_name;
    m_acceptor = std::make_unique<tcp::acceptor>(io_context, tcp::endpoint(tcp::v4(), m_port));

    Config::CommunicationSettings communication_settings = config.get_communication_settings();
    m_timeout_seconds = std::chrono::seconds(communication_settings.socket_timeout);

    std::cout << "SmtpServer initialized and listening on port " << m_port << std::endl;
}

void SmtpServer::Start() { Accept(); }

void SmtpServer::Accept() {
    auto new_socket = std::make_shared<TcpSocket>(m_io_context);

    m_acceptor->async_accept(*new_socket, [this, new_socket](const boost::system::error_code& error) {
        if (!error) {
            std::cout << "Accepted new connection." << std::endl;
            // Dispatch the client handling to the thread pool
            m_thread_pool.EnqueueDetach([this, new_socket]() { HandleClient(SocketWrapper(new_socket)); });
        } else {
            ErrorHandler::HandleBoostError("Accept", error);
        }
        Accept();
    });
}

void SmtpServer::ResetTimeoutTimer(SocketWrapper& socket_wrapper) {
	auto timeout_timer = std::make_shared<boost::asio::steady_timer>(m_io_context);
	socket_wrapper.SetTimeoutTimer(timeout_timer);
	socket_wrapper.StartTimeoutTimer(m_timeout_seconds);
}

void SmtpServer::HandleClient(SocketWrapper socket_wrapper) {
    try {
        MailMessageBuilder mail_builder;
        std::string current_line;

        while (true) {
            if (!socket_wrapper.IsOpen()) break;
            std::cout << socket_wrapper.IsOpen() << std::endl;
            auto future_data = socket_wrapper.ReadFromSocketAsync(MAX_LENGTH);

			try {
				std::string buffer = future_data.get();
				current_line.append(buffer);
				std::size_t pos;

                while ((pos = current_line.find("\r\n")) != std::string::npos) {
                    std::string line = current_line.substr(0, pos);
                    current_line.erase(0, pos + 2);
                    m_command_handler.ProcessLine(line, socket_wrapper);
                }

				socket_wrapper.CancelTimeoutTimer();
				ResetTimeoutTimer(socket_wrapper);
			} catch (const boost::system::system_error& e) {
                if (e.code() == boost::asio::error::operation_aborted) {
                    std::cout << "Client disconnected." << std::endl;
                    break;
                }
                ErrorHandler::HandleException("Read from socket", e);
                throw;
            } catch (const std::exception& e) {
                std::cerr << "Read from socket error: " << e.what() << std::endl;
                throw;
            }
        }
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}
}