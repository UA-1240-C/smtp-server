#include "Server.h"

#include <array>
#include <boost/beast/core/detail/base64.hpp>  // For Base64 decoding
#include <ctime>
#include <fstream>
#include <iostream>
#include <utility>

#include "MailMessageBuilder.h"

using namespace ISXSC;

// Constructor for the SmtpServer class
SmtpServer::SmtpServer(boost::asio::io_context& io_context,
                       boost::asio::ssl::context& ssl_context,
                       const unsigned short port, ThreadPool<>& thread_pool)
    : io_context_(io_context),
      ssl_context_(ssl_context),
      acceptor_(std::make_unique<tcp::acceptor>(
          io_context, tcp::endpoint(tcp::v4(), port))),
      thread_pool_(thread_pool),
      in_data_(0) {
    ssl_context_.set_options(boost::asio::ssl::context::default_workarounds |
                             boost::asio::ssl::context::no_sslv2 |
                             boost::asio::ssl::context::no_sslv3 |
                             boost::asio::ssl::context::no_tlsv1 |
                             boost::asio::ssl::context::no_tlsv1_1);
    std::cout << "SmtpServer initialized and listening on port " << port
              << std::endl;
}

void SmtpServer::Start() { Accept(); }

void SmtpServer::Accept() {
    auto new_socket = std::make_shared<TcpSocket>(io_context_);

    acceptor_->async_accept(
        *new_socket,
        [this, new_socket](const boost::system::error_code& error) {
            if (!error) {
                std::cout << "Accepted new connection." << std::endl;
                // Dispatch the client handling to the thread pool
                thread_pool_.enqueue_detach([this, new_socket]() {
                    handleClient(SocketWrapper(new_socket));
                });
            } else {
                handleBoostError("Accept", error);
            }
            Accept();
        });
}

void SmtpServer::sendResponse(SocketWrapper socket_wrapper,
                              const std::string& response) {
    if (socket_wrapper.is_tls()) {
        async_write(
            *socket_wrapper.get<SslSocket>(), boost::asio::buffer(response),
            [this](const boost::system::error_code& error, std::size_t) {
                if (error) {
                    handleBoostError("Write", error);
                }
            });
    } else {
        async_write(
            *socket_wrapper.get<TcpSocket>(), boost::asio::buffer(response),
            [this](const boost::system::error_code& error, std::size_t) {
                if (error) {
                    handleBoostError("Write", error);
                }
            });
    }
}

void SmtpServer::handleClient(SocketWrapper socket_wrapper) {
    try {
        bool in_data = false;
        MailMessageBuilder mail_builder;
        std::array<char, 1024> buffer;
        std::string current_line;

        while (true) {
            boost::system::error_code error;
            size_t length{};
            readFromSocket(socket_wrapper, buffer, length, error);

            if (error) {
                handleBoostError("Read from socket", error);
                if (error == boost::asio::error::eof) {
                    std::cout << "Client disconnected." << std::endl;
                    break;
                }
                throw boost::system::system_error(error);
            }

            current_line.append(buffer.data(), length);
            std::size_t pos;

            while ((pos = current_line.find("\r\n")) != std::string::npos) {
                std::string line = current_line.substr(0, pos);
                current_line.erase(0, pos + 2);
                processLine(line, socket_wrapper, in_data, mail_builder);
            }
        }
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}

void SmtpServer::readFromSocket(SocketWrapper& socket_wrapper,
                                std::array<char, 1024>& buffer, size_t& length,
                                boost::system::error_code& error) {
    if (socket_wrapper.is_tls()) {
        auto ssl_socket =
            socket_wrapper
                .get<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>();
        if (ssl_socket) {
            length = ssl_socket->read_some(boost::asio::buffer(buffer), error);
        }
    } else {
        auto tcp_socket = socket_wrapper.get<boost::asio::ip::tcp::socket>();
        if (tcp_socket) {
            length = tcp_socket->read_some(boost::asio::buffer(buffer), error);
        }
    }
}

void SmtpServer::processLine(const std::string& line,
                             SocketWrapper& socket_wrapper, bool& in_data,
                             MailMessageBuilder& mail_builder) {
    if (line.find("EHLO") == 0) {
        handleEhlo(socket_wrapper);
    } else if (line.find("MAIL FROM:") == 0) {
        handleMailFrom(socket_wrapper, line);
    } else if (line.find("RCPT TO:") == 0) {
        handleRcptTo(socket_wrapper, line);
    } else if (line.find("DATA") == 0) {
        handleData(socket_wrapper);
    } else if (line.find("QUIT") == 0) {
        handleQuit(socket_wrapper);
    } else if (line.find("NOOP") == 0) {
        handleNoop(socket_wrapper);
    } else if (line.find("RSET") == 0) {
        handleRset(socket_wrapper);
    } else if (line.find("HELP") == 0) {
        handleHelp(socket_wrapper);
    } else if (line.find("STARTTLS") == 0) {
        handleStartTLS(socket_wrapper);
        // Wait for async handshake to complete
        throw std::runtime_error("Waiting for async handshake to complete");
    } else if (line.find("AUTH LOGIN") == 0) {
        handleAuth(socket_wrapper, line);
    } else {
        tempHandleDataMode(line, mail_builder, socket_wrapper, in_data);
    }
}

void SmtpServer::tempHandleDataMode(const std::string& line,
                                    MailMessageBuilder& mail_builder,
                                    SocketWrapper& socket_wrapper,
                                    bool& in_data) {
    if (in_data) {
        saveData(line, mail_builder, socket_wrapper, in_data);
    } else {
        sendResponse(socket_wrapper, "500 Command not recognized\r\n");
    }
}

void SmtpServer::handleBoostError(
    const std::string where, const boost::system::error_code& error) const {
    std::cerr << where << " error: " << error.message() << std::endl;
}

void SmtpServer::saveData(const std::string& line,
                          MailMessageBuilder& mail_builder,
                          SocketWrapper socket_wrapper, bool& in_data) {
    if (line == ".") {
        in_data = false;
        tempSaveMail(mail_builder.Build());
        sendResponse(std::move(socket_wrapper), "250 OK\r\n");
    } else {
        // mail_builder.AddBody(line);
    }
}

void SmtpServer::handleEhlo(SocketWrapper socket_wrapper) {
    sendResponse(std::move(socket_wrapper), "250 Hello\r\n");
}

void SmtpServer::handleMailFrom(SocketWrapper socket_wrapper,
                                const std::string& line) {
    mail_builder_.SetFrom(line.substr(10));
    sendResponse(std::move(socket_wrapper), "250 OK\r\n");
}

void SmtpServer::handleRcptTo(SocketWrapper socket_wrapper,
                              const std::string& line) {
    mail_builder_.AddTo(line.substr(9));
    sendResponse(std::move(socket_wrapper), "250 OK\r\n");
}
void SmtpServer::handleData(SocketWrapper socket_wrapper) {
    // Send a reply to the client to start receiving data
    sendResponse(socket_wrapper, "354 End data with <CR><LF>.<CR><LF>\r\n");
    in_data_ = true;

    std::string data_message;

    try {
        while (in_data_) {
            readData(socket_wrapper, data_message);
            processDataMessage(data_message, socket_wrapper);
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception during DATA handling: " << e.what()
                  << std::endl;
        throw;
    }
}

void SmtpServer::readData(SocketWrapper& socket_wrapper,
                          std::string& data_message) {
    boost::system::error_code error;
    std::array<char, 1024> buffer;
    size_t length;

    // Reading data from a socket
    readFromSocket(socket_wrapper, buffer, length, error);

    if (error) {
        handleBoostError("Read Data", error);
        if (error == boost::asio::error::eof) {
            std::cout << "Client disconnected." << std::endl;
            in_data_ = false;
        } else {
            throw boost::system::system_error(error);
        }
    }

    // Adding read data to a buffer
    data_message.append(buffer.data(), length);
}

void SmtpServer::processDataMessage(std::string& data_message,
                                    SocketWrapper& socket_wrapper) {
    std::size_t pos;
    while ((pos = data_message.find("\r\n")) != std::string::npos) {
        std::string line = data_message.substr(0, pos);
        data_message.erase(0, pos + 2);

        // Перевірка на закінчення даних
        if (line == ".") {
            handleEndOfData(socket_wrapper);
            break;
        }

        // Adding a string to the message constructor
        mail_builder_.AddData(line);
    }
}

void SmtpServer::handleEndOfData(SocketWrapper& socket_wrapper) {
    // Completion of data processing
    in_data_ = false;
    sendResponse(socket_wrapper, "250 OK\r\n");

    // Outputting the full DATA message to the console
    std::cout << "Full DATA message:\n" << mail_builder_.GetBody() << std::endl;
}

void SmtpServer::handleNoop(SocketWrapper socket_wrapper) {
    sendResponse(std::move(socket_wrapper), "250 OK\r\n");
}

void SmtpServer::handleRset(SocketWrapper socket_wrapper) {
    mail_builder_ = MailMessageBuilder();
    sendResponse(std::move(socket_wrapper), "250 OK\r\n");
}

void SmtpServer::handleHelp(SocketWrapper socket_wrapper) {
    sendResponse(std::move(socket_wrapper),
                 "214 The following commands are recognized: "
                 "HELO, MAIL FROM, RCPT TO, DATA, "
                 "QUIT, NOOP, RSET, VRFY, HELP\r\n");
}

void SmtpServer::handleStartTLS(SocketWrapper socket_wrapper) {
    // Upgrade the connection to TLS
    auto tcp_socket = socket_wrapper.get<TcpSocket>();
    auto ssl_socket =
        std::make_shared<SslSocket>(std::move(*tcp_socket), ssl_context_);
    socket_wrapper = SocketWrapper(ssl_socket);

    ssl_socket->async_handshake(
        boost::asio::ssl::stream_base::server,
        [this, ssl_socket](const boost::system::error_code& error) {
            if (!error) {
                sendResponse(SocketWrapper(ssl_socket),
                             "220 Ready to start TLS\r\n");
            } else {
                sendResponse(SocketWrapper(ssl_socket),
                             "454 TLS negotiation failed\r\n");
            }
        });
}

void SmtpServer::handleAuth(SocketWrapper socket_wrapper,
                            const std::string& line) {
    // Waitinf for AUTH LOGIN base64(username):base64(password)
    std::string base64_data = line.substr(5);  // Erase "AUTH "
    std::string decoded_data = ISXBase64::Base64Decode(base64_data);

    std::size_t pos = decoded_data.find(':');
    if (pos == std::string::npos) {
        sendResponse(std::move(socket_wrapper),
                     "501 Syntax error in parameters or arguments\r\n");
        return;
    }

    std::string username = decoded_data.substr(0, pos);
    std::string password = decoded_data.substr(pos + 1);

	/*
    // Should be implemented Hash/Validate
    if (validateCredentials(username, password)) {
        sendResponse(std::move(socket_wrapper),
                     "235 Authentication successful\r\n");
    } else {
        sendResponse(std::move(socket_wrapper),
                     "535 Authentication failed\r\n");
    }

	*/
}

void SmtpServer::handleQuit(SocketWrapper socket_wrapper) {
    sendResponse(socket_wrapper, "221 Bye\r\n");

    if (socket_wrapper.is_tls()) {
        handleQuitSsl(socket_wrapper);
    } else {
        handleQuitTcp(socket_wrapper);
    }

    std::cout << "Connection closed by client." << std::endl;
    throw std::runtime_error("Client disconnected");
}

void SmtpServer::handleQuitSsl(SocketWrapper socket_wrapper) {
    auto ssl_socket = socket_wrapper.get<SslSocket>();
    if (ssl_socket) {
        boost::system::error_code error;
        ssl_socket->shutdown(error);
        if (error) {
            std::cerr << "Error during SSL shutdown: " << error.message()
                      << std::endl;
        }
        ssl_socket->lowest_layer().close(error);
        if (error) {
            std::cerr << "Error closing SSL socket: " << error.message()
                      << std::endl;
        }
    }
}

void SmtpServer::handleQuitTcp(SocketWrapper socket_wrapper) {
    auto tcp_socket = socket_wrapper.get<TcpSocket>();
    if (tcp_socket) {
        boost::system::error_code error;
        tcp_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both,
                             error);
        if (error) {
            std::cerr << "Error during TCP shutdown: " << error.message()
                      << std::endl;
        }
        tcp_socket->close(error);
        if (error) {
            std::cerr << "Error closing TCP socket: " << error.message()
                      << std::endl;
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
        std::cerr << "Failed to save mail: " << e.what() << std::endl;
    }
}

std::string SmtpServer::tempCreateFileName() const {
    std::ostringstream file_name_stream;
    file_name_stream << "mail_" << std::time(nullptr) << ".eml";
    return file_name_stream.str();
}

std::ofstream SmtpServer::tempOpenFile(const std::string& file_name) const {
    std::ofstream output_file(file_name, std::ios::out | std::ios::binary);
    if (!output_file) {
        throw std::runtime_error("Failed to open file for writing: " +
                                 file_name);
    }
    return std::move(output_file);
}

void SmtpServer::tempWriteEmailContent(std::ofstream& output_file,
                                       const MailMessage& message) const {
    output_file << "From: " << message.from_ << "\r\n";
    for (const auto& to : message.to_) {
        output_file << "To: " << to << "\r\n";
    }
    for (const auto& cc : message.cc_) {
        output_file << "Cc: " << cc << "\r\n";
    }
    for (const auto& bcc : message.bcc_) {
        output_file << "Bcc: " << bcc << "\r\n";
    }
    output_file << "Subject: " << message.subject_ << "\r\n";
    output_file << "\r\n";  // Empty line between headers and body
    output_file << message.body_ << "\r\n";
}

void SmtpServer::tempWriteAttachments(std::ofstream& output_file,
                                      const MailMessage& message) const {
    for (const auto& attachment : message.attachments_) {
        std::ifstream attachment_file(attachment.get_path(),
                                      std::ios::in | std::ios::binary);
        if (!attachment_file) {
            throw std::runtime_error("Failed to open attachment: " +
                                     attachment.get_path().string());
        }

        output_file << "\r\n--boundary\r\n";
        output_file << "Content-Type: application/octet-stream\r\n";
        output_file << "Content-Disposition: attachment; filename=\""
                    << attachment.get_name() << "\"\r\n";
        output_file << "Content-Transfer-Encoding: base64\r\n";
        output_file << "\r\n";

        std::ostringstream base64_stream;
        base64_stream << attachment_file.rdbuf();

        // std::string base64_data = base64_encode(base64_stream.str()); //
        // Implement base64_encode function output_file << base64_data <<
        // "\r\n";
    }
}

/*

std::string SmtpServer::hashPassword(const std::string& password) {
    const auto salt = "$2a$12$";  // Here $2a$12$ means algotithm bcrypt and the
                                                              // cost of the
hashing(for example, 12) std::string hashed_password(crypt(password.c_str(),
    salt));

    return hashed_password;
}

// temporarily commented

bool SmtpServer::validateCredentials(const std::string& username,
                                     const std::string& password) {
    try {
        auto it = user_db_.find(username);
        if (it == user_db_.end()) return false;

        std::string stored_hash = it->second;

        std::string password_hash =
            crypt(password.c_str(), stored_hash.c_str());
        return password_hash == stored_hash;
    } catch (const std::exception& e) {
        std::cerr << "Error validating credentials: " << e.what() << std::endl;
        return false;
    }
}

*/