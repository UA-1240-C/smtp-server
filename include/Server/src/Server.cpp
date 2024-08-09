#include "Server.h"

constexpr std::size_t MAX_LENGTH = 1024;

#include <sodium.h>

#include <array>
#include <ctime>
#include <fstream>
#include <iostream>

#include "../../ErrorHandler/include/ErrorHandler.h"
#include "config.h"

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
    m_timeout_timer.cancel();

    m_timeout_timer.expires_after(std::chrono::seconds(m_timeout_seconds));

    m_timeout_timer.async_wait([this, &socket_wrapper](const boost::system::error_code& error) {
        if (error) {
            return;
        }

        try {
            std::cout << "Client timed out." << std::endl;
            socket_wrapper.Close();
        } catch (const std::exception& e) {
            std::cerr << "Exception in timeout handler: " << e.what() << std::endl;
            ErrorHandler::HandleException("Timeout handler", e);
        }
    });
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