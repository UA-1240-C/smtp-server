#include "Server.h"

#include <sodium.h>

#include <array>
#include <ctime>
#include <fstream>
#include <iostream>

#include "../../ErrorHandler/include/ErrorHandler.h"
#include "config.h"

using namespace ISXSC;

// Constructor for the SmtpServer class
SmtpServer::SmtpServer(boost::asio::io_context& io_context, boost::asio::ssl::context& ssl_context)
    : m_io_context_(io_context),
      m_ssl_context_(ssl_context),
      m_command_handler_(ssl_context),
      m_timeout_timer_(io_context),
      m_thread_pool_([&] {
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
    m_acceptor_ = std::make_unique<tcp::acceptor>(io_context, tcp::endpoint(tcp::v4(), m_port));

    Config::CommunicationSettings communication_settings = config.get_communication_settings();
    m_timeout_timer_.expires_after(std::chrono::seconds(communication_settings.socket_timeout));

    std::cout << "SmtpServer initialized and listening on port " << m_port << std::endl;
}

void SmtpServer::Start() { Accept(); }

void SmtpServer::Accept() {
    auto new_socket = std::make_shared<TcpSocket>(m_io_context_);

    m_acceptor_->async_accept(*new_socket, [this, new_socket](const boost::system::error_code& error) {
        if (!error) {
            std::cout << "Accepted new connection." << std::endl;
            // Dispatch the client handling to the thread pool
            m_thread_pool_.enqueue_detach([this, new_socket]() { HandleClient(SocketWrapper(new_socket)); });
        } else {
            ErrorHandler::handleBoostError("Accept", error);
        }
        Accept();
    });
}

void SmtpServer::HandleClient(SocketWrapper socket_wrapper) {
    try {
        bool in_data = false;
        MailMessageBuilder mail_builder;
        std::string current_line;

        while (true) {
            size_t length = 1024;
            auto future_data = socket_wrapper.readFromSocketAsync(length);

            try {
                std::string buffer = future_data.get();
                current_line.append(buffer);
                std::size_t pos;

                while ((pos = current_line.find("\r\n")) != std::string::npos) {
                    std::string line = current_line.substr(0, pos);
                    current_line.erase(0, pos + 2);
                    m_command_handler_.ProcessLine(line, socket_wrapper);
                }
            } catch (const std::exception& e) {
                ErrorHandler::handleException("Read from socket", e);
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

void SmtpServer::tempHandleDataMode(const std::string& line, MailMessageBuilder& mail_builder,
                                    SocketWrapper& socket_wrapper, bool& in_data) {
    if (in_data) {
        saveData(line, mail_builder, socket_wrapper, in_data);
    } else {
        try {
            socket_wrapper.sendResponseAsync("500 Command not recognized\r\n").get();
        } catch (const std::exception& e) {
            ErrorHandler::handleException("Handle Data Mode", e);
        }
    }
}

void SmtpServer::saveData(const std::string& line, MailMessageBuilder& mail_builder, SocketWrapper& socket_wrapper,
                          bool& in_data) {
    if (line == ".") {
        in_data = false;
        try {
            tempSaveMail(mail_builder.Build());

            socket_wrapper.sendResponseAsync("250 OK\r\n").get();
        } catch (const std::exception& e) {
            // Обработка ошибок
            std::cerr << "Error while saving data or sending response: " << e.what() << std::endl;
        }
    }
}
// Save the email message to a file
void SmtpServer::tempSaveMail(const MailMessage& message) {
    try {
        std::string file_name = tempCreateFileName();
        std::ofstream output_file = tempOpenFile(file_name);

        tempWriteEmailContent(output_file, message);
        tempWriteAttachments(output_file, message);

        output_file.close();
        std::cout << "Mail saved to " << file_name << std::endl;

    } catch (const std::exception& e) {
        ErrorHandler::handleException("Save mail", e);
    }
}

std::string SmtpServer::tempCreateFileName() const {
    std::ostringstream file_name_stream;

    // Getting the current time
    std::time_t current_time = std::time(nullptr);
    if (current_time == -1) {
        std::cerr << "Error: Failed to get current time\n";
        return "";
    }

    file_name_stream << "mail_" << current_time << ".eml";

    std::string file_name = file_name_stream.str();
    if (file_name.empty()) {
        std::cerr << "Error: Failed to create file name\n";
        return "";
    }

    return file_name;
}

std::ofstream SmtpServer::tempOpenFile(const std::string& file_name) const {
    try {
        std::ofstream output_file(file_name, std::ios::out | std::ios::binary);
        if (!tempIsOutputFileValid(output_file)) {
            std::cerr << "Failed to open file for writing\n";
            return std::ofstream();
        }
        return output_file;  // std::move is not necessary

    } catch (const std::exception& e) {
        ErrorHandler::handleException("Open file", e);
        return std::ofstream();
    }
}

bool SmtpServer::tempIsOutputFileValid(const std::ofstream& output_file) const {
    if (!output_file) {
        std::cerr << "Invalid output file stream\n";
        return false;
    }
    return true;
}

void SmtpServer::tempWriteEmailContent(std::ofstream& output_file, const MailMessage& message) const {
    if (!tempIsOutputFileValid(output_file)) return;

    /*
        The overload << operator is not implemented
        output_file << "From: " << message.from << "\r\n";
    */
    for (const auto& to : message.to) {
        // output_file << "To: " << to << "\r\n";
    }
    for (const auto& cc : message.cc) {
        // output_file << "Cc: " << cc << "\r\n";
    }
    for (const auto& bcc : message.bcc) {
        // output_file << "Bcc: " << bcc << "\r\n";
    }
    // output_file << "Subject: " << message.subject << "\r\n";
    // output_file << "\r\n";  // Empty line between headers and body
    // output_file << message.body << "\r\n";
}

void SmtpServer::tempWriteAttachments(std::ofstream& output_file, const MailMessage& message) const {
    if (!tempIsOutputFileValid(output_file)) return;

    for (const auto& attachment : message.attachments) {
        std::ifstream attachment_file(attachment.get_path(), std::ios::in | std::ios::binary);
        if (!attachment_file) {
            std::cerr << "Failed to open attachment: " << attachment.get_path().string() << "\n";
            continue;
        }

        output_file << "\r\n--boundary\r\n";
        output_file << "Content-Type: application/octet-stream\r\n";
        output_file << "Content-Disposition: attachment; filename=\"" << attachment.get_name() << "\"\r\n";
        output_file << "Content-Transfer-Encoding: base64\r\n";
        output_file << "\r\n";

        std::ostringstream base64_stream;
        base64_stream << attachment_file.rdbuf();

        // std::string base64_data = base64_encode(base64_stream.str()); //
        // Implement base64_encode function output_file << base64_data <<
        // "\r\n";
    }
}