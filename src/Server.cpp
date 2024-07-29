#include "Server.h"
#include <array>
#include <ctime>
#include <chrono>
#include <iostream>

// Save the email message to a file
void SmtpServer::save_mail(const MailData& mail_data) {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto in_time_t = system_clock::to_time_t(now);

    std::tm tm = *std::localtime(&in_time_t);
    char filename[100];
    std::strftime(filename, sizeof(filename), "mail_%Y%m%d_%H%M%S.txt", &tm);

    std::ofstream file(filename);
    if (file.is_open()) {
        file << "From: " << mail_data.sender_ << "\n";
        file << "To: ";
        for (const auto& recipient : mail_data.recipients_) {
            file << recipient << " ";
        }
        file << "\n";
        file << "Message:\n" << mail_data.message_ << "\n";
        file.close();
    } else {
        std::cerr << "Failed to open file for writing" << std::endl;
    }
}

// Handle incoming requests
void SmtpServer::handle_request(std::shared_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> socket, bool is_tls) {
    try {
        std::array<char, 1024> buffer;
        std::string response;
        std::string command;
        MailData mail_data;
        bool in_data = false;

        for (;;) {
            boost::system::error_code error;
            std::size_t length = socket->read_some(boost::asio::buffer(buffer), error);

            if (error == boost::asio::error::eof) {
                std::cout << "Client disconnected." << std::endl;
                break;
            } else if (error) {
                throw boost::system::system_error(error);
            }

            command.append(buffer.data(), length);

            std::size_t pos;
            while ((pos = command.find("\r\n")) != std::string::npos) {
                std::string line = command.substr(0, pos);
                command.erase(0, pos + 2);

                std::cout << "Received line: " << line << std::endl;

                if (line.find("HELO") == 0) {
                    response = "250 Hello\r\n";
                } else if (line.find("MAIL FROM:") == 0) {
                    mail_data.sender_ = line.substr(10);
                    response = "250 OK\r\n";
                } else if (line.find("RCPT TO:") == 0) {
                    mail_data.recipients_.push_back(line.substr(9));
                    response = "250 OK\r\n";
                } else if (line.find("DATA") == 0) {
                    response = "354 End data with <CR><LF>.<CR><LF>\r\n";
                    in_data = true;
                    mail_data.message_.clear(); // Clear message before receiving
                } else if (line.find("QUIT") == 0) {
                    response = "221 Bye\r\n";
                    socket->shutdown();
                    std::cout << "Connection closed by client." << std::endl;
                    return; // End the handling
                } else if (line.find("NOOP") == 0) {
                    response = "250 OK\r\n";
                } else if (line.find("RSET") == 0) {
                    mail_data = MailData(); // Reset data
                    response = "250 OK\r\n";
                } else if (line.find("HELP") == 0) {
                    response = "214 The following commands are recognized: HELO, MAIL FROM, RCPT TO, DATA, QUIT, NOOP, RSET, VRFY, HELP\r\n";
                } else if (line.find("STARTTLS") == 0) {
                    if (!is_tls) {
                        response = "220 Ready to start TLS\r\n";
                        boost::asio::write(*socket, boost::asio::buffer(response), error);
                        if (error)
                            throw boost::system::system_error(error); // Write error

                        // Perform TLS handshake
                        socket->async_handshake(boost::asio::ssl::stream_base::server,
                            [socket, this](const boost::system::error_code& error) {
                                if (!error) {
                                    std::cout << "TLS handshake successful." << std::endl;
                                    // Continue handling requests after successful handshake
                                    handle_request(socket, true); // Call handle_request with is_tls = true
                                } else {
                                    std::cerr << "TLS handshake failed: " << error.message() << std::endl;
                                }
                            });
                        return; // Wait for async handshake to complete
                    } else {
                        response = "454 TLS not available\r\n";
                    }
                } else if (in_data) {
                    if (line == ".") {
                        response = "250 OK\r\n";
                        in_data = false;
                        save_mail(mail_data); // Save message to file
                    } else {
                        mail_data.message_ += line + "\n";
                        response = ""; // Do not send response yet
                    }
                } else {
                    response = "500 Command not recognized\r\n";
                }

                if (!response.empty()) {
                    boost::asio::write(*socket, boost::asio::buffer(response), error);
                    if (error)
                        throw boost::system::system_error(error); // Write error
                    response.clear(); // Clear response after sending
                }
            }
        }

        // Close connection
        socket->shutdown();
        std::cout << "Connection closed." << std::endl;

    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}

// Constructor for the SmtpServer class
SmtpServer::SmtpServer(boost::asio::io_context& io_context, unsigned short port, ThreadPool<>& pool)
    : acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
      pool_(pool),
      ssl_context_(boost::asio::ssl::context::tlsv12_server) {

    ssl_context_.use_certificate_chain_file("/home/shevmee/server.pem");
    ssl_context_.use_private_key_file("/home/shevmee/server.key", boost::asio::ssl::context::pem);

    start_accept();
    std::cout << "Server is running and listening on port " << port << std::endl;
}

// Start accepting incoming connections
void SmtpServer::start_accept() {
    auto socket = std::make_shared<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>(acceptor_.get_executor(), ssl_context_);
    acceptor_.async_accept(socket->lowest_layer(),
        [this, socket](const boost::system::error_code& error) {
            if (!error) {
                // Perform handshake if STARTTLS is requested
                handle_session(socket);
            } else {
                std::cerr << "Accept error: " << error.message() << std::endl;
            }
            start_accept();
        });
}

// Handle a new session
void SmtpServer::handle_session(std::shared_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> socket) {
    pool_.enqueue_detach([socket, this]() mutable {
        handle_request(socket, false);
    });
}
