#include "Server.h"

#include <array>
#include <ctime>
#include <fstream>
#include <iostream>
#include <utility>

#include <sodium.h>

#include "MailMessageBuilder.h"

using namespace ISXSC;

// Constructor for the SmtpServer class
SmtpServer::SmtpServer(boost::asio::io_context &io_context,
                       boost::asio::ssl::context &ssl_context,
                       const unsigned short port, ThreadPool<> &thread_pool)
    : thread_pool_(thread_pool), io_context_(io_context),
      ssl_context_(ssl_context),
      acceptor_(std::make_unique<tcp::acceptor>(
          io_context, tcp::endpoint(tcp::v4(), port))),
      data_base_(std::make_unique<ISXMailDB::PgMailDB>("localhost")) {
  ssl_context_.set_options(boost::asio::ssl::context::default_workarounds |
                           boost::asio::ssl::context::no_sslv2 |
                           boost::asio::ssl::context::no_sslv3 |
                           boost::asio::ssl::context::no_tlsv1 |
                           boost::asio::ssl::context::no_tlsv1_1);
  std::cout << "SmtpServer initialized and listening on port " << port
            << std::endl;

  try {
    ConnectToDatabase();
  } catch (const ISXMailDB::MailException &e) {
    std::cerr << "MailException: " << e.what() << std::endl;
    throw; // Re-throw to ensure proper exception handling at creation time
  } catch (const std::exception &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    throw; // Re-throw to ensure proper exception handling at creation time
  }
}

SmtpServer::~SmtpServer() {
  try {
    DisconnectFromDatabase();
  } catch (const ISXMailDB::MailException &e) {
    std::cerr << "MailException during disconnect: " << e.what() << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Exception during disconnect: " << e.what() << std::endl;
  }
}

void SmtpServer::ConnectToDatabase() {
  data_base_->Connect(connection_string_);
  if (!data_base_->IsConnected()) {
    throw std::runtime_error("Database connection is not established.");
  }
}

void SmtpServer::DisconnectFromDatabase() {
  if (data_base_->IsConnected()) {
    data_base_->Disconnect();
  }
}

void SmtpServer::Start() { Accept(); }

void SmtpServer::Accept() {
  auto new_socket = std::make_shared<TcpSocket>(io_context_);

  acceptor_->async_accept(
      *new_socket, [this, new_socket](const boost::system::error_code &error) {
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
                              const std::string &response) {
  if (socket_wrapper.is_tls()) {
    async_write(*socket_wrapper.get<SslSocket>(), boost::asio::buffer(response),
                [this](const boost::system::error_code &error, std::size_t) {
                  if (error) {
                    handleBoostError("Write", error);
                  }
                });
  } else {
    async_write(*socket_wrapper.get<TcpSocket>(), boost::asio::buffer(response),
                [this](const boost::system::error_code &error, std::size_t) {
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
  } catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }
}

void SmtpServer::readFromSocket(SocketWrapper &socket_wrapper,
                                std::array<char, 1024> &buffer, size_t &length,
                                boost::system::error_code &error) {
  if (socket_wrapper.is_tls()) {
    auto ssl_socket = socket_wrapper.get<SslSocket>();
    if (ssl_socket) {
      length = ssl_socket->read_some(boost::asio::buffer(buffer), error);
    }
  } else {
    auto tcp_socket = socket_wrapper.get<TcpSocket>();
    if (tcp_socket) {
      length = tcp_socket->read_some(boost::asio::buffer(buffer), error);
    }
  }
}

void SmtpServer::processLine(const std::string &line,
                             SocketWrapper &socket_wrapper, bool &in_data,
                             MailMessageBuilder &mail_builder) {
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
  } else if (line.find("REGISTER") == 0) {
    handleRegister(socket_wrapper, line);
  } else {
    tempHandleDataMode(line, mail_builder, socket_wrapper, in_data);
  }
}

void SmtpServer::tempHandleDataMode(const std::string &line,
                                    MailMessageBuilder &mail_builder,
                                    SocketWrapper &socket_wrapper,
                                    bool &in_data) {
  if (in_data) {
    saveData(line, mail_builder, socket_wrapper, in_data);
  } else {
    sendResponse(socket_wrapper, "500 Command not recognized\r\n");
  }
}

void SmtpServer::handleBoostError(
    const std::string where, const boost::system::error_code &error) const {
  std::cerr << where << " error: " << error.message() << std::endl;
}

void SmtpServer::saveData(const std::string &line,
                          MailMessageBuilder &mail_builder,
                          SocketWrapper socket_wrapper, bool &in_data) {
  if (line == ".") {
    in_data = false;
    tempSaveMail(mail_builder.Build());
    sendResponse(std::move(socket_wrapper), "250 OK\r\n");
  }
}

void SmtpServer::handleEhlo(SocketWrapper socket_wrapper) {
  sendResponse(std::move(socket_wrapper), "250 Hello\r\n");
}

void SmtpServer::handleMailFrom(SocketWrapper socket_wrapper,
                                const std::string &line) {
  std::string sender = line.substr(10);
  try {
    if (!data_base_->UserExists(sender)) {
      sendResponse(socket_wrapper, "550 Sender address does not exist\r\n");
      return;
    }
    mail_builder_.SetFrom(sender);
    sendResponse(std::move(socket_wrapper), "250 OK\r\n");
  } catch (const std::exception &e) {
    handleException("Handle MAIL FROM", e);
    sendResponse(socket_wrapper, "550 Internal Server Error\r\n");
  }
}

void SmtpServer::handleRcptTo(SocketWrapper socket_wrapper,
                              const std::string &line) {
  std::string recipient = line.substr(8);
  try {
    if (!data_base_->UserExists(recipient)) {
      sendResponse(socket_wrapper, "550 Recipient address does not exist\r\n");
      return;
    }
    mail_builder_.AddTo(recipient);
    sendResponse(socket_wrapper, "250 OK\r\n");
  } catch (const std::exception &e) {
    handleException("Handle RCPT TO", e);
    sendResponse(std::move(socket_wrapper), "550 Internal Server Error\r\n");
  }
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
  } catch (const std::exception &e) {
    handleException("Data handling", e);
    throw;
  }
}

void SmtpServer::readData(SocketWrapper &socket_wrapper,
                          std::string &data_message) {
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

void SmtpServer::processDataMessage(std::string &data_message,
                                    SocketWrapper &socket_wrapper) {
  std::size_t pos;
  while ((pos = data_message.find("\r\n")) != std::string::npos) {
    std::string line = data_message.substr(0, pos);
    data_message.erase(0, pos + 2);

    if (line == ".") {
      handleEndOfData(socket_wrapper);
      break;
    }

    // Adding a string to the message constructor
    mail_builder_.AddData(line);
  }
}

void SmtpServer::handleEndOfData(SocketWrapper &socket_wrapper) {
  in_data_ = false;
  sendResponse(socket_wrapper, "250 OK\r\n");

  try {
    MailMessage message = mail_builder_.Build();
    if (message.from_.get_address().empty() || message.to_.empty()) {
      sendResponse(socket_wrapper, "550 Required fields missing\r\n");
    } else {
      SaveMailToDatabase(message);
      std::cout << "Full DATA message:\n"
                << mail_builder_.GetBody() << std::endl;
    }
  } catch (const std::exception &e) {
    handleException("Build and Save Mail", e);
    sendResponse(socket_wrapper, "550 Internal Server Error\r\n");
  }

  mail_builder_ = MailMessageBuilder();
}

void SmtpServer::SaveMailToDatabase(const MailMessage &message) {
  try {
    for (const auto &recipient : message.to_) {
      try {
        data_base_->InsertEmail(message.from_.get_address(),
                                recipient.get_address(), message.subject_,
                                message.body_);
      } catch (const std::exception &e) {
        std::cerr << "Failed to insert email: " << e.what() << std::endl;
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "Database error: " << e.what() << std::endl;
    throw;
  }
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

void SmtpServer::handleException(std::string where,
                                 const std::exception &e) const {
  std::cerr << where << " error: " << e.what() << "\n";
}

void SmtpServer::handleStartTLS(SocketWrapper socket_wrapper) {

  sendResponse(socket_wrapper, "220 Ready to start TLS\r\n");

  // Upgrade the socket to use TLS
  socket_wrapper.upgradeToTls(ssl_context_);

  // Wait for the TLS handshake to complete
  // This can be improved by adding proper async handling if necessary
  throw std::runtime_error("Waiting for async handshake to complete");
  /*std::cout << "Handling STARTTLS command." << std::endl;

  if (socket_wrapper.is_tls()) {
      sendResponse(socket_wrapper, "503 Bad sequence of commands\r\n");
      return;
  }

  auto tcp_socket = socket_wrapper.get<TcpSocket>();
  auto ssl_socket = std::make_shared<SslSocket>(std::move(*tcp_socket),
  ssl_context_); socket_wrapper = SocketWrapper(ssl_socket);

  std::cout << "Starting TLS handshake." << std::endl;

  ssl_socket->async_handshake(
      boost::asio::ssl::stream_base::server,
      [this, &socket_wrapper](const boost::system::error_code& error) {
          if (!error) {
              std::cout << "TLS handshake completed successfully." << std::endl;
              sendResponse(socket_wrapper, "220 Ready to start TLS\r\n");
              // Start handling the client with the new SSL socket
              handleClient(socket_wrapper);
          } else {
              std::cerr << "TLS handshake failed: " << error.message() <<
  std::endl; sendResponse(socket_wrapper, "454 TLS negotiation failed\r\n");
          }
      });*/
}

void SmtpServer::handleAuth(SocketWrapper socket_wrapper,
                            const std::string &line) {
  /*std::string encoded_credentials = line.substr(11);
  std::string decoded_credentials =
  ISXBase64::Base64Decode(encoded_credentials);

  std::cout << "Decoded credentials: " << decoded_credentials << std::endl;

  auto delimiter_pos = decoded_credentials.find(':');
  if (delimiter_pos == std::string::npos) {
      sendResponse(socket_wrapper, "501 Syntax error in parameters or
  arguments\r\n"); return;
  }

  std::string email = decoded_credentials.substr(0, delimiter_pos);
  std::string password = decoded_credentials.substr(delimiter_pos + 1);

  std::cout << "Email part: " << email << std::endl;
  std::cout << "Password part: " << password << std::endl;

  try {
      std::string hashed_password = hashPassword(password);
      std::cout << "Hashed password: " << hashed_password << std::endl;

      data_base_->Login(email, hashed_password);
      sendResponse(socket_wrapper, "235 Authentication successful\r\n");
  } catch (const ISXMailDB::MailException& e) {
      handleException("Handle AUTH", e);
      sendResponse(socket_wrapper, "535 Authentication failed\r\n");
  } catch (const std::exception& e) {
      handleException("Handle AUTH", e);
      sendResponse(socket_wrapper, "535 Internal Server Error\r\n");
  }*/
}

std::string SmtpServer::hashPassword(const std::string &password) {
  std::string hashed_password;
  std::vector<unsigned char> hash(crypto_pwhash_STRBYTES);

  if (crypto_pwhash_str(reinterpret_cast<char *>(hash.data()), password.c_str(),
                        password.size(), crypto_pwhash_OPSLIMIT_INTERACTIVE,
                        crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
    throw std::runtime_error("Password hashing failed");
  }

  hashed_password.assign(hash.begin(), hash.end());
  return hashed_password;
}

void SmtpServer::handleRegister(SocketWrapper socket_wrapper,
                                const std::string &line) {
  std::string user_details = line.substr(9);
  std::string decoded_user_details = ISXBase64::Base64Decode(user_details);

  std::cout << "Decoded user details: " << decoded_user_details << std::endl;

  size_t delimiter_pos = decoded_user_details.find(':');
  if (delimiter_pos == std::string::npos) {
    sendResponse(socket_wrapper,
                 "501 Syntax error in parameters or arguments\r\n");
    return;
  }

  std::string username = decoded_user_details.substr(0, delimiter_pos);
  std::string password = decoded_user_details.substr(delimiter_pos + 1);

  std::cout << "Username part: " << username << std::endl;
  std::cout << "Password part: " << password << std::endl;

  try {
    if (data_base_->UserExists(username)) {
      sendResponse(socket_wrapper, "550 User already exists\r\n");
    } else {
      std::string hashed_password = hashPassword(password);
      std::cout << "Hashed password: " << hashed_password << std::endl;

      data_base_->SignUp(username, hashed_password);
      sendResponse(socket_wrapper, "250 User registered successfully\r\n");
    }
  } catch (const ISXMailDB::MailException &e) {
    handleException("Handle REGISTER", e);
    sendResponse(socket_wrapper, "550 Registration failed\r\n");
  } catch (const std::exception &e) {
    handleException("Handle REGISTER", e);
    sendResponse(socket_wrapper, "550 Internal Server Error\r\n");
  }
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
      handleBoostError("SSL shutdown", error);
    }

    ssl_socket->lowest_layer().close(error);

    if (error) {
      handleBoostError("SSL closing socket", error);
    }
  }
}

void SmtpServer::handleQuitTcp(SocketWrapper socket_wrapper) {
  auto tcp_socket = socket_wrapper.get<TcpSocket>();
  if (tcp_socket) {
    boost::system::error_code error;
    tcp_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
    if (error) {
      handleBoostError("TCP shutdown", error);
    }

    tcp_socket->close(error);

    if (error) {
      handleBoostError("TCP closing socket", error);
    }
  }
}

// Save the email message to a file
void SmtpServer::tempSaveMail(const MailMessage &message) {
  try {
    std::string file_name = tempCreateFileName();
    std::ofstream output_file = tempOpenFile(file_name);

    tempWriteEmailContent(output_file, message);
    tempWriteAttachments(output_file, message);

    output_file.close();
    std::cout << "Mail saved to " << file_name << std::endl;

  } catch (const std::exception &e) {
    handleException("Save mail", e);
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

std::ofstream SmtpServer::tempOpenFile(const std::string &file_name) const {
  try {
    std::ofstream output_file(file_name, std::ios::out | std::ios::binary);
    if (!tempIsOutputFileValid(output_file)) {
      std::cerr << "Failed to open file for writing\n";
      return std::ofstream();
    }
    return output_file; // std::move is not necessary

  } catch (const std::exception &e) {
    handleException("Open file", e);
    return std::ofstream();
  }
}

bool ISXSC::SmtpServer::tempIsOutputFileValid(
    const std::ofstream &output_file) const {
  if (!output_file) {
    std::cerr << "Invalid output file stream\n";
    return false;
  }
  return true;
}

void SmtpServer::tempWriteEmailContent(std::ofstream &output_file,
                                       const MailMessage &message) const {
  if (!tempIsOutputFileValid(output_file))
    return;

  output_file << "From: " << message.from_ << "\r\n";
  for (const auto &to : message.to_) {
    output_file << "To: " << to << "\r\n";
  }
  for (const auto &cc : message.cc_) {
    output_file << "Cc: " << cc << "\r\n";
  }
  for (const auto &bcc : message.bcc_) {
    output_file << "Bcc: " << bcc << "\r\n";
  }
  output_file << "Subject: " << message.subject_ << "\r\n";
  output_file << "\r\n"; // Empty line between headers and body
  output_file << message.body_ << "\r\n";
}

void SmtpServer::tempWriteAttachments(std::ofstream &output_file,
                                      const MailMessage &message) const {
  if (!tempIsOutputFileValid(output_file))
    return;

  for (const auto &attachment : message.attachments_) {
    std::ifstream attachment_file(attachment.get_path(),
                                  std::ios::in | std::ios::binary);
    if (!attachment_file) {
      std::cerr << "Failed to open attachment: "
                << attachment.get_path().string() << "\n";
      continue;
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
|