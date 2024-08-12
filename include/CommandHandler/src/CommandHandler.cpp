#include <iostream>

#include "CommandHandler.h"
#include "ErrorHandler.h"

using namespace ISXSC;

CommandHandler::CommandHandler(boost::asio::ssl::context& ssl_context)
    : ssl_context_(ssl_context)
    , m_data_base_(std::make_unique<ISXMailDB::PgMailDB>("localhost"))
{
    ssl_context_.set_options(boost::asio::ssl::context::default_workarounds |
        boost::asio::ssl::context::no_sslv2 |
        boost::asio::ssl::context::no_sslv3 |
        boost::asio::ssl::context::no_tlsv1 |
        boost::asio::ssl::context::no_tlsv1_1);

    try
    {
        ConnectToDatabase();
    }
    catch (const ISXMailDB::MailException& e)
    {
        std::cerr << "MailException: " << e.what() << std::endl;
        throw; // Re-throw to ensure proper exception handling at creation time
    } catch (const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        throw; // Re-throw to ensure proper exception handling at creation time
    }
}

CommandHandler::~CommandHandler()
{
    try {
        DisconnectFromDatabase();
    } catch (const ISXMailDB::MailException& e) {
        std::cerr << "MailException during disconnect: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception during disconnect: " << e.what() << std::endl;
    }
}

void CommandHandler::ConnectToDatabase() const
{
    m_data_base_->Connect(connection_string_);
    if (!m_data_base_->IsConnected()) {
        throw std::runtime_error("Database connection is not established.");
    }
}

void CommandHandler::DisconnectFromDatabase() const
{
    if (m_data_base_->IsConnected()) {
        m_data_base_->Disconnect();
    }
}

void CommandHandler::ProcessLine(const std::string& line, SocketWrapper& socket_wrapper) {
  if (line.find("EHLO") == 0) {
    HandleEhlo(socket_wrapper);
  } else if (line.find("MAIL FROM:") == 0) {
    HandleMailFrom(socket_wrapper, line);
  } else if (line.find("RCPT TO:") == 0) {
    HandleRcptTo(socket_wrapper, line);
  } else if (line.find("DATA") == 0) {
      HandleData(socket_wrapper);
  } else if (line.find("QUIT") == 0) {
    HandleQuit(socket_wrapper);
  } else if (line.find("NOOP") == 0) {
    HandleNoop(socket_wrapper);
  } else if (line.find("RSET") == 0) {
    HandleRset(socket_wrapper);
  } else if (line.find("HELP") == 0) {
    HandleHelp(socket_wrapper);
  } else if (line.find("VRFY") == 0) {
    HandleVrfy(socket_wrapper, line);
  } else if (line.find("EXPN") == 0) {
      HandleExpn(socket_wrapper, line);
  } else if (line.find("STARTTLS") == 0) {
      HandleStartTLS(socket_wrapper);
  } else if (line.find("AUTH PLAIN") == 0) {
    HandleAuth(socket_wrapper, line);
  } else if (line.find("REGISTER") == 0) {
    HandleRegister(socket_wrapper, line);
  } else {
      auto future = socket_wrapper.sendResponseAsync("500 Syntax error, command unrecognized\r\n");
      try {
          future.get(); // Ожидание завершения асинхронной операции
          std::cout << "Response sent successfully." << std::endl;
      } catch (const std::exception& e) {
          std::cerr << "Error sending response: " << e.what() << std::endl;
      }

    // Assuming tempHandleDataMode is a member function,
    // update it to use the member mail_builder_
    // tempHandleDataMode(line, mail_builder, socket_wrapper, in_data);
  }
}

void CommandHandler::HandleEhlo(SocketWrapper& socket_wrapper) {
    auto future = socket_wrapper.sendResponseAsync("250 Hello\r\n");
    try {
        future.get();
    } catch (const std::exception& e) {
        ErrorHandler::handleException("Handle EHLO", e);
    }
}

void CommandHandler::HandleNoop(SocketWrapper& socket_wrapper) {
    auto future = socket_wrapper.sendResponseAsync("250 OK\r\n");
    try {
        future.get();
    } catch (const std::exception& e) {
        ErrorHandler::handleException("Handle NOOP", e);
    }
}

void CommandHandler::HandleRset(SocketWrapper& socket_wrapper) {
    m_mail_builder_ = MailMessageBuilder();
    auto future = socket_wrapper.sendResponseAsync("250 OK\r\n");
    try {
        future.get();
    } catch (const std::exception& e) {
        ErrorHandler::handleException("Handle RSET", e);
    }
}

void CommandHandler::HandleHelp(SocketWrapper& socket_wrapper) {
    auto future = socket_wrapper.sendResponseAsync(
        "214 The following commands are recognized: "
        "HELO, MAIL FROM, RCPT TO, DATA, "
        "QUIT, NOOP, RSET, VRFY, HELP\r\n"
    );
    try {
        future.get();
    } catch (const std::exception& e) {
        ErrorHandler::handleException("Handle HELP", e);
    }
}

void CommandHandler::HandleVrfy(SocketWrapper& socket_wrapper, const std::string& line)
{
    std::string user_name = line.substr(5); // Remove "VRFY " (5 characters)

    try {
        if (m_data_base_->UserExists(user_name)) {
            auto user_info_list = m_data_base_->RetrieveUserInfo(user_name);

            std::string response = "250 User exists: " + user_name + "\r\n";
            for (const auto& user : user_info_list) {
                response += "Full name: " + user.user_name + user.host_name + "\r\n";
            }
            auto future = socket_wrapper.sendResponseAsync(response);
            future.get();
        } else {
            auto future = socket_wrapper.sendResponseAsync("550 User does not exist\r\n");
            future.get();
        }
    } catch (const ISXMailDB::MailException& e) {
        ErrorHandler::handleError("Handle VRFY", e, socket_wrapper, "550 Internal Server Error\r\n");
    }
}

void CommandHandler::HandleExpn(SocketWrapper& socket_wrapper, const std::string& line)
{
    std::string mailing_list = line.substr(5); // Remove "EXPN " (5 characters)

    try {
        auto members_list = m_data_base_->RetrieveUserInfo(mailing_list);

        if (!members_list.empty()) {
            std::string response = "250 Mailing list members:\r\n";
            for (const auto& member : members_list) {
                response += member.user_name + "\r\n";
            }
            auto future = socket_wrapper.sendResponseAsync(response);
            future.get();
        } else {
            auto future = socket_wrapper.sendResponseAsync("550 Mailing list does not exist\r\n");
            future.get();
        }
    } catch (const ISXMailDB::MailException& e) {
        ErrorHandler::handleError("Handle EXPN", e, socket_wrapper, "550 Internal Server Error\r\n");
    }
}

void CommandHandler::HandleQuit(SocketWrapper& socket_wrapper) {
    auto future = socket_wrapper.sendResponseAsync("221 Bye\r\n");
    try {
        future.get();
    } catch (const std::exception& e) {
        ErrorHandler::handleException("Handle QUIT", e);
    }

    if (socket_wrapper.is_tls()) {
        HandleQuitSsl(socket_wrapper);
    } else {
        HandleQuitTcp(socket_wrapper);
    }

    std::cout << "Connection closed by client." << std::endl;
    throw std::runtime_error("Client disconnected");
}

void CommandHandler::HandleQuitSsl(SocketWrapper& socket_wrapper) {
    auto ssl_socket = socket_wrapper.get<SslSocket>();
    if (ssl_socket) {
        boost::system::error_code error;
        ssl_socket->shutdown(error);
        if (error) {
            ErrorHandler::handleBoostError("SSL shutdown", error);
        }

        ssl_socket->lowest_layer().close(error);
        if (error) {
            ErrorHandler::handleBoostError("SSL closing socket", error);
        }
    }
}

void CommandHandler::HandleQuitTcp(SocketWrapper& socket_wrapper) {
    auto tcp_socket = socket_wrapper.get<TcpSocket>();
    if (tcp_socket) {
        boost::system::error_code error;
        tcp_socket->shutdown(TcpSocket::shutdown_both, error);
        if (error) {
            ErrorHandler::handleBoostError("TCP shutdown", error);
        }

        tcp_socket->close(error);
        if (error) {
            ErrorHandler::handleBoostError("TCP closing socket", error);
        }
    }
}

void CommandHandler::HandleMailFrom(SocketWrapper& socket_wrapper, const std::string& line) {
    std::cout << "Handling MAIL FROM command with line: " << line << std::endl;
    if (line.size() <= 10) {
        std::cerr << "Invalid MAIL FROM command format" << std::endl;
        auto future = socket_wrapper.sendResponseAsync("501 Syntax error in parameters or arguments\r\n");
        try {
            future.get();
        } catch (const std::exception& e) {
            std::cerr << "Exception in handleMailFrom: " << e.what() << std::endl;
        }
        return;
    }

    const std::string sender = line.substr(10);
    std::cout << "Parsed sender: " << sender << std::endl;

    try {
        if (!m_data_base_->UserExists(sender)) {
            std::cout << "Sender does not exist" << std::endl;
            auto future = socket_wrapper.sendResponseAsync("550 Sender address does not exist\r\n");
            future.get();
        } else {
            m_mail_builder_.SetFrom(sender);
            std::cout << "Sender address set" << std::endl;
            auto future = socket_wrapper.sendResponseAsync("250 OK\r\n");
            future.get();
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception in handleMailFrom: " << e.what() << std::endl;
        ErrorHandler::handleException("Handle MAIL FROM", e);
        auto future = socket_wrapper.sendResponseAsync("550 Internal Server Error\r\n");
        try {
            future.get();
        } catch (const std::exception& e) {
            std::cerr << "Exception in handleMailFrom: " << e.what() << std::endl;
        }
    }
}

void CommandHandler::HandleRcptTo(SocketWrapper& socket_wrapper,
                                  const std::string& line) {
    const std::string recipient = line.substr(8);
    try {
        if (!m_data_base_->UserExists(recipient)) {
            auto future = socket_wrapper.sendResponseAsync("550 Recipient address does not exist\r\n");
            future.get();
            return;
        }
        m_mail_builder_.AddTo(recipient);
        auto future = socket_wrapper.sendResponseAsync("250 OK\r\n");
        future.get();
    } catch (const std::exception& e) {
        ErrorHandler::handleException("Handle RCPT TO", e);
        try {
            auto future = socket_wrapper.sendResponseAsync("550 Internal Server Error\r\n");
            future.get();
        } catch (const std::exception& e) {
            ErrorHandler::handleException("Send Internal Server Error response", e);
        }
    }
}

void CommandHandler::HandleData(SocketWrapper& socket_wrapper) {
    auto future = socket_wrapper.sendResponseAsync("354 End data with <CR><LF>.<CR><LF>\r\n");
    try {
        future.get();
        m_in_data_ = true;

        while (m_in_data_) {
            std::string data_message;
            ReadData(socket_wrapper, data_message);
            ProcessDataMessage(socket_wrapper, data_message);
        }
    } catch (const std::exception& e) {
        ErrorHandler::handleException("Data handling", e);
        throw;
    }
}

void CommandHandler::ReadData(SocketWrapper& socket_wrapper, std::string& data_message) {
    auto future_data = socket_wrapper.readFromSocketAsync(1024);

    try {
        std::string buffer = future_data.get();
        data_message.append(buffer);

    } catch (const std::exception& e) {
        ErrorHandler::handleException("Read Data", e);
        if (dynamic_cast<const boost::system::system_error*>(&e)) {
            std::cout << "Client disconnected." << std::endl;
            m_in_data_ = false;
        } else {
            throw;
        }
    }
}

void CommandHandler::ProcessDataMessage(SocketWrapper& socket_wrapper,
                                    std::string& data_message) {
    std::size_t pos;
    while ((pos = data_message.find("\r\n")) != std::string::npos) {
        std::string line = data_message.substr(0, pos);
        data_message.erase(0, pos + 2);

        if (line == ".") {
            HandleEndOfData(socket_wrapper);
            break;
        }

        m_mail_builder_.SetBody(line);
    }
}

void CommandHandler::HandleEndOfData(SocketWrapper& socket_wrapper) {
    m_in_data_ = false;
    try {
        auto future_response = socket_wrapper.sendResponseAsync("250 OK\r\n");
        future_response.get();

        try {
            MailMessage message = m_mail_builder_.Build();
            if (message.from.get_address().empty() || message.to.empty()) {
                future_response = socket_wrapper.sendResponseAsync("550 Required fields missing\r\n");
            } else {
                SaveMailToDatabase(message);
                future_response = socket_wrapper.sendResponseAsync("250 OK\r\n");
            }
            future_response.get();
        } catch (const std::exception& e) {
            ErrorHandler::handleException("Build and Save Mail", e);
            future_response = socket_wrapper.sendResponseAsync("550 Internal Server Error\r\n");
            future_response.get();
        }

        m_mail_builder_ = MailMessageBuilder();
    } catch (const std::exception& e) {
        ErrorHandler::handleException("Handle End Of Data", e);
    }
}

void CommandHandler::SaveMailToDatabase(const MailMessage& message)
{
  try {
    for (const auto& recipient : message.to) {
      try {
        m_data_base_->InsertEmail(
            message.from.get_address(),
            recipient.get_address(),
            message.subject,
            message.body);
      } catch (const std::exception& e) {
        std::cerr << "Failed to insert email: " << e.what() << std::endl;
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Database error: " << e.what() << std::endl;
    throw;
  }
}

void CommandHandler::HandleStartTLS(SocketWrapper& socket_wrapper) {
    if (socket_wrapper.is_tls()) {
        socket_wrapper.sendResponseAsync("503 Already in TLS mode.\r\n").get();
        return;
    }

    auto future_response = socket_wrapper.sendResponseAsync("220 Ready to start TLS\r\n");

    try {
        future_response.get();

        auto future_tls = socket_wrapper.startTlsAsync(ssl_context_);
        future_tls.get();

        std::cout << "STARTTLS handshake completed successfully." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception during STARTTLS handshake: " << e.what() << std::endl;
        socket_wrapper.sendResponseAsync("530 STARTTLS handshake failed\r\n").get();
    }
}

void CommandHandler::HandleAuth(SocketWrapper& socket_wrapper, const std::string& line) {
    try {
        auto [username, password] = DecodeAndSplitPlain(line.substr(11));
        std::cout << "Username exists: " << m_data_base_->UserExists(username) << std::endl;

        if (!m_data_base_->UserExists(username)) {
            std::cout << "User " << username << " with password " << password << " does not exist\n";
            socket_wrapper.sendResponseAsync("535 Authentication failed\r\n").get();
            return;
        }

        std::string stored_hashed_password = m_data_base_->GetPasswordHash(username);
        if (!VerifyPassword(password, stored_hashed_password)) {
            throw std::runtime_error("Authentication failed");
        }

        socket_wrapper.sendResponseAsync("235 Authentication successful\r\n").get();
    } catch (const std::runtime_error& e) {
        ErrorHandler::handleError("Handle AUTH", e, socket_wrapper, "535 Authentication failed\r\n");
    } catch (const ISXMailDB::MailException& e) {
        ErrorHandler::handleError("Handle AUTH", e, socket_wrapper, "535 Authentication failed\r\n");
    } catch (const std::exception& e) {
        ErrorHandler::handleError("Handle AUTH", e, socket_wrapper, "535 Internal Server Error\r\n");
    }
}

void CommandHandler::HandleRegister(SocketWrapper& socket_wrapper, const std::string& line) {
    try {
        auto [username, password] = DecodeAndSplitPlain(line.substr(9));

        if (m_data_base_->UserExists(username)) {
            socket_wrapper.sendResponseAsync("550 User already exists\r\n").get();
            return;
        }

        std::string hashed_password = HashPassword(password);
        m_data_base_->SignUp(username, hashed_password);
        socket_wrapper.sendResponseAsync("250 User registered successfully\r\n").get();
    } catch (const std::runtime_error& e) {
        ErrorHandler::handleError("Handle REGISTER", e, socket_wrapper, "501 Syntax error in parameters or arguments\r\n");
    } catch (const ISXMailDB::MailException& e) {
        ErrorHandler::handleError("Handle REGISTER", e, socket_wrapper, "550 Registration failed\r\n");
    } catch (const std::exception& e) {
        ErrorHandler::handleError("Handle REGISTER", e, socket_wrapper, "550 Internal Server Error\r\n");
    }
}

std::pair<std::string, std::string> CommandHandler::DecodeAndSplitPlain(const std::string& encoded_data) {
    std::string decoded_data = ISXBase64::Base64Decode(encoded_data);
    std::cout << "Decoded data: " << decoded_data << std::endl;

    size_t first_null = decoded_data.find('\0');
    if (first_null == std::string::npos) {
        throw std::runtime_error("Invalid PLAIN format: Missing first null byte.");
    }
    std::cout << "First null byte at position: " << first_null << std::endl;

    size_t second_null = decoded_data.find('\0', first_null + 1);
    if (second_null == std::string::npos) {
        throw std::runtime_error("Invalid PLAIN format: Missing second null byte.");
    }
    std::cout << "Second null byte at position: " << second_null << std::endl;

    std::string username = decoded_data.substr(first_null + 1, second_null - first_null - 1);
    std::string password = decoded_data.substr(second_null + 1);

    std::cout << "Extracted username: " << username << std::endl;
    std::cout << "Extracted password: " << password << std::endl;

    return {username, password};
}


bool CommandHandler::VerifyPassword(const std::string& password,
                                    const std::string& hashed_password) {
    return crypto_pwhash_str_verify(hashed_password.c_str(), password.c_str(),
                                    password.size()) == 0;
}

std::string CommandHandler::HashPassword(const std::string& password) {
    std::string hashed_password(crypto_pwhash_STRBYTES, '\0');

    if (crypto_pwhash_str(hashed_password.data(), password.c_str(),
                          password.size(), crypto_pwhash_OPSLIMIT_INTERACTIVE,
                          crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
        throw std::runtime_error("Password hashing failed");
                          }

    return hashed_password;
}