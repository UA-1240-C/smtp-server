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
    : io_context_(io_context),
      ssl_context_(ssl_context),
      command_handler_(ssl_context),
      timeout_timer_(io_context),
      thread_pool_([&] {
          Config config("../config.txt");
          Config::ThreadPool thread_pool_config = config.get_thread_pool();
          size_t max_threads = thread_pool_config.max_working_threads > std::thread::hardware_concurrency()
                                   ? std::thread::hardware_concurrency()
                                   : thread_pool_config.max_working_threads;
          return ThreadPool<>(max_threads);
      }()) {
    Config config("../config.txt");
    Config::Server server = config.get_server();
    port = server.listener_port;
    server_display_name = server.server_display_name;
    server_name = server.server_name;
    acceptor_ = std::make_unique<tcp::acceptor>(io_context, tcp::endpoint(tcp::v4(), port));

    Config::CommunicationSettings communication_settings = config.get_communication_settings();
    timeout_seconds_ = std::chrono::seconds(communication_settings.socket_timeout);

    std::cout << "SmtpServer initialized and listening on port " << port << std::endl;
}

void SmtpServer::Start() { Accept(); }

void SmtpServer::Accept() {
    auto new_socket = std::make_shared<TcpSocket>(m_io_context);

    acceptor_->async_accept(*new_socket, [this, new_socket](const boost::system::error_code& error) {
        if (!error) {
            std::cout << "Accepted new connection." << std::endl;
            // Dispatch the client handling to the thread pool
            thread_pool_.enqueue_detach([this, new_socket]() { handleClient(SocketWrapper(new_socket)); });
        } else {
            ErrorHandler::handleBoostError("Accept", error);
        }
        Accept();
    });
}

void SmtpServer::resetTimeoutTimer(SocketWrapper& socket_wrapper) {
    timeout_timer_.cancel();

    timeout_timer_.expires_after(std::chrono::seconds(timeout_seconds_));

    timeout_timer_.async_wait([this, &socket_wrapper](const boost::system::error_code& error) {
        if (error) {
            return;
        }

        try {
            std::cout << "Client timed out." << std::endl;
            socket_wrapper.close(); 
        } catch (const std::exception& e) {
            std::cerr << "Exception in timeout handler: " << e.what() << std::endl;
            ErrorHandler::handleException("Timeout handler", e);
        }
    });
}

void SmtpServer::HandleClient(SocketWrapper socket_wrapper) {
    try {
        MailMessageBuilder mail_builder;
        std::string current_line;

        while (true) {
            if (!socket_wrapper.is_open()) break;
            std::cout << socket_wrapper.is_open() << std::endl;
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

                resetTimeoutTimer(socket_wrapper);

            } catch (const boost::system::system_error& e) {
                if (e.code() == boost::asio::error::operation_aborted) {
                    std::cout << "Client disconnected." << std::endl;
                    break;
                }
                ErrorHandler::handleException("Read from socket", e);
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