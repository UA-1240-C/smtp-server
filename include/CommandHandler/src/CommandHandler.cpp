#include <iostream>

#include "CommandHandler.h"
#include "../../ErrorHandler/include/ErrorHandler.h"

using namespace ISXSC;

CommandHandler::CommandHandler(boost::asio::ssl::context& ssl_context)
    : ssl_context_(ssl_context)
    , data_base_(std::make_unique<ISXMailDB::PgMailDB>("localhost"))
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

void CommandHandler::ConnectToDatabase() {
    data_base_->Connect(connection_string_);
    if (!data_base_->IsConnected()) {
        throw std::runtime_error("Database connection is not established.");
    }
}

void CommandHandler::DisconnectFromDatabase() {
    if (data_base_->IsConnected()) {
        data_base_->Disconnect();
    }
}

void CommandHandler::processLine(const std::string& line, SocketWrapper& socket_wrapper,
                                 bool in_data, MailMessageBuilder& mail_builder) {
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
  } else if (line.find("VRFY") == 0) {
    handleVrfy(socket_wrapper, line);
  } else if (line.find("EXPN") == 0) {
      handleExpn(socket_wrapper, line);
  } else if (line.find("STARTTLS") == 0) {
      handleStartTLS(socket_wrapper);
  } else if (line.find("AUTH LOGIN") == 0) {
    handleAuth(socket_wrapper, line);
  } else if (line.find("REGISTER") == 0) {
    handleRegister(socket_wrapper, line);
  } else {
    socket_wrapper.sendResponse("500 Syntax error, command unrecognized\r\n");

    // Assuming tempHandleDataMode is a member function,
    // update it to use the member mail_builder_
    // tempHandleDataMode(line, mail_builder, socket_wrapper, in_data);
  }
}

void CommandHandler::handleEhlo(SocketWrapper& socket_wrapper) {
    try {
        socket_wrapper.sendResponse("250 Hello\r\n");
    } catch (const std::exception& e) {
        ErrorHandler::handleException("Handle EHLO", e);
    }
}

void CommandHandler::handleNoop(SocketWrapper& socket_wrapper) {
    try {
        socket_wrapper.sendResponse("250 OK\r\n");
    } catch (const std::exception& e) {
        ErrorHandler::handleException("Handle NOOP", e);
    }
}

void CommandHandler::handleRset(SocketWrapper& socket_wrapper) {
    mail_builder_ = MailMessageBuilder();
    try {
        socket_wrapper.sendResponse("250 OK\r\n");
    } catch (const std::exception& e) {
        ErrorHandler::handleException("Handle RSET", e);
    }
}

void CommandHandler::handleHelp(SocketWrapper& socket_wrapper) {
    try {
        socket_wrapper.sendResponse(
            "214 The following commands are recognized: "
            "HELO, MAIL FROM, RCPT TO, DATA, "
            "QUIT, NOOP, RSET, VRFY, HELP\r\n"
        );
    } catch (const std::exception& e) {
        ErrorHandler::handleException("Handle HELP", e);
    }
}

void CommandHandler::handleVrfy(SocketWrapper& socket_wrapper, const std::string& line)
{
    std::string user_name = line.substr(5); // Remove "VRFY " (5 characters)

    try {
        if (data_base_->UserExists(user_name)) {
            // Retrieve additional user information if available
            auto user_info_list = data_base_->RetrieveUserInfo(user_name);

            std::string response = "250 User exists: " + user_name + "\r\n";
            for (const auto& user : user_info_list) {
                response += "Full name: " + user.user_name + user.host_name + "\r\n";
                // You can include other details if necessary, e.g., user.host_name
            }
            socket_wrapper.sendResponse(response);
        } else {
            socket_wrapper.sendResponse("550 User does not exist\r\n");
        }
    } catch (const ISXMailDB::MailException& e) {
        ErrorHandler::handleError("Handle VRFY", e, socket_wrapper, "550 Internal Server Error\r\n");
    }
}

void CommandHandler::handleExpn(SocketWrapper& socket_wrapper, const std::string& line)
{
    std::string mailing_list = line.substr(5); // Remove "EXPN " (5 characters)

    try {
        // Retrieve list membership information
        auto members_list = data_base_->RetrieveUserInfo(mailing_list); // Adjust if necessary for lists

        if (!members_list.empty()) {
            std::string response = "250 Mailing list members:\r\n";
            for (const auto& member : members_list) {
                response += member.user_name + "\r\n"; // Adjust if necessary for list members
            }
            socket_wrapper.sendResponse(response);
        } else {
            socket_wrapper.sendResponse("550 Mailing list does not exist\r\n");
        }
    } catch (const ISXMailDB::MailException& e) {
        ErrorHandler::handleError("Handle EXPN", e, socket_wrapper, "550 Internal Server Error\r\n");
    }
}

void CommandHandler::handleQuit(SocketWrapper& socket_wrapper) {
    try {
        socket_wrapper.sendResponse("221 Bye\r\n");
    } catch (const std::exception& e) {
        ErrorHandler::handleException("Handle QUIT", e);
    }

    if (socket_wrapper.is_tls()) {
        handleQuitSsl(socket_wrapper);
    } else {
        handleQuitTcp(socket_wrapper);
    }

    std::cout << "Connection closed by client." << std::endl;
    throw std::runtime_error("Client disconnected");
}

void CommandHandler::handleQuitSsl(SocketWrapper& socket_wrapper) {
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

void CommandHandler::handleQuitTcp(SocketWrapper& socket_wrapper) {
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

void CommandHandler::handleMailFrom(SocketWrapper& socket_wrapper, const std::string& line) {
    std::cout << "Handling MAIL FROM command with line: " << line << std::endl;
    const std::string sender = line.substr(10);
    std::cout << "Parsed sender: " << sender << std::endl;

    try {
        if (!data_base_->UserExists(sender)) {
            std::cout << "Sender does not exist" << std::endl;
            socket_wrapper.sendResponse("550 Sender address does not exist\r\n");
        } else {
            mail_builder_.SetFrom(sender);
            std::cout << "Sender address set" << std::endl;
            socket_wrapper.sendResponse("250 OK\r\n");
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception in handleMailFrom: " << e.what() << std::endl;
        ErrorHandler::handleException("Handle MAIL FROM", e);
        socket_wrapper.sendResponse("550 Internal Server Error\r\n");
    }
}

void CommandHandler::handleRcptTo(SocketWrapper& socket_wrapper,
                                  const std::string& line) {
    const std::string recipient = line.substr(8);
    try {
        if (!data_base_->UserExists(recipient)) {
            socket_wrapper.sendResponse("550 Recipient address does not exist\r\n");
            return;
        }
        mail_builder_.AddTo(recipient);
        socket_wrapper.sendResponse("250 OK\r\n");
    } catch (const std::exception& e) {
        ErrorHandler::handleException("Handle RCPT TO", e);
        try {
            socket_wrapper.sendResponse("550 Internal Server Error\r\n");
        } catch (const std::exception& e) {
            ErrorHandler::handleException("Send Internal Server Error response", e);
        }
    }
}

void CommandHandler::handleData(SocketWrapper& socket_wrapper) {
    try {
        socket_wrapper.sendResponse("354 End data with <CR><LF>.<CR><LF>\r\n");
        in_data_ = true;

        while (in_data_) {
            std::string data_message;
            readData(socket_wrapper, data_message);
            processDataMessage(socket_wrapper, data_message);
        }
    } catch (const std::exception& e) {
        ErrorHandler::handleException("Data handling", e);
        throw;
    }
}

void CommandHandler::readData(SocketWrapper& socket_wrapper, std::string& data_message)
{
    boost::system::error_code error;
    std::array<char, 1024> buffer{};
    size_t length;

    // Reading data from a socket
    readFromSocket(socket_wrapper, buffer, length, error);

    if (error) {
        ErrorHandler::handleBoostError("Read Data", error);
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

void CommandHandler::processDataMessage(SocketWrapper& socket_wrapper,
                                    std::string& data_message) {
    std::size_t pos;
    while ((pos = data_message.find("\r\n")) != std::string::npos) {
        std::string line = data_message.substr(0, pos);
        data_message.erase(0, pos + 2);

        // Check for end of data
        if (line == ".") {
            handleEndOfData(socket_wrapper);
            break;
        }

        // Adding a string to the message builder
        mail_builder_.SetBody(line);
    }
}

void CommandHandler::handleEndOfData(SocketWrapper& socket_wrapper) {
    in_data_ = false;
    try {
        socket_wrapper.sendResponse("250 OK\r\n");

        try {
            MailMessage message = mail_builder_.Build();
            if (message.from.get_address().empty() || message.to.empty()) {
                socket_wrapper.sendResponse("550 Required fields missing\r\n");
            } else {
                saveMailToDatabase(message);
                std::cout << "Full DATA message: " << std::endl;
            }
        } catch (const std::exception& e) {
            ErrorHandler::handleException("Build and Save Mail", e);
            socket_wrapper.sendResponse("550 Internal Server Error\r\n");
        }

        mail_builder_ = MailMessageBuilder();
    } catch (const std::exception& e) {
        ErrorHandler::handleException("Handle End Of Data", e);
    }
}

void CommandHandler::readFromSocket(SocketWrapper& socket_wrapper,
                                std::array<char, 1024>& buffer, size_t& length,
                                boost::system::error_code& error) {
  if (socket_wrapper.is_tls()) {
    if (auto ssl_socket = socket_wrapper.get<SslSocket>()) {
      length = ssl_socket->read_some(boost::asio::buffer(buffer), error);
    }
  } else {
    if (auto tcp_socket = socket_wrapper.get<TcpSocket>()) {
      length = tcp_socket->read_some(boost::asio::buffer(buffer), error);
    }
  }
}

void CommandHandler::saveMailToDatabase(const MailMessage& message)
{
  try {
    for (const auto& recipient : message.to) {
      try {
        data_base_->InsertEmail(
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

void CommandHandler::handleStartTLS(SocketWrapper& socket_wrapper) {
    if (socket_wrapper.is_tls()) {
        socket_wrapper.sendResponse("503 Already in TLS mode.\r\n");
        return;
    }

    socket_wrapper.sendResponse("220 Ready to start TLS\r\n");
    socket_wrapper.start_tls(ssl_context_);
}

void CommandHandler::handleAuth(SocketWrapper& socket_wrapper, const std::string& line) {
    try {
        auto [username, password] = decodeAndSplitPlain(line.substr(11));
        std::string hashed_password = hashPassword(password);
        data_base_->Login(username, hashed_password);
        socket_wrapper.sendResponse("235 Authentication successful\r\n");
    } catch (const std::runtime_error& e) {
        ErrorHandler::handleError("Handle AUTH", e, socket_wrapper, "501 Syntax error in parameters or arguments\r\n");
    } catch (const ISXMailDB::MailException& e) {
        ErrorHandler::handleError("Handle AUTH", e, socket_wrapper, "535 Authentication failed\r\n");
    } catch (const std::exception& e) {
        ErrorHandler::handleError("Handle AUTH", e, socket_wrapper, "535 Internal Server Error\r\n");
    }
}

void CommandHandler::handleRegister(SocketWrapper& socket_wrapper, const std::string& line) {
    try {
        auto [username, password] = decodeAndSplitPlain(line.substr(9));
        if (data_base_->UserExists(username)) {
            socket_wrapper.sendResponse("550 User already exists\r\n");
        } else {
            std::string hashed_password = hashPassword(password);
            data_base_->SignUp(username, hashed_password);
            socket_wrapper.sendResponse("250 User registered successfully\r\n");
        }
    } catch (const std::runtime_error& e) {
        ErrorHandler::handleError("Handle REGISTER", e, socket_wrapper, "501 Syntax error in parameters or arguments\r\n");
    } catch (const ISXMailDB::MailException& e) {
        ErrorHandler::handleError("Handle REGISTER", e, socket_wrapper, "550 Registration failed\r\n");
    } catch (const std::exception& e) {
        ErrorHandler::handleError("Handle REGISTER", e, socket_wrapper, "550 Internal Server Error\r\n");
    }
}

std::pair<std::string, std::string> CommandHandler::decodeAndSplitPlain(const std::string& encoded_data) {
    std::string decoded_data = ISXBase64::Base64Decode(encoded_data);

    size_t first_null = decoded_data.find('\0');
    if (first_null == std::string::npos) {
        throw std::runtime_error("Invalid PLAIN format: Missing first null byte.");
    }

    size_t second_null = decoded_data.find('\0', first_null + 1);
    if (second_null == std::string::npos) {
        throw std::runtime_error("Invalid PLAIN format: Missing second null byte.");
    }

    std::string username = decoded_data.substr(first_null + 1, second_null - first_null - 1);
    std::string password = decoded_data.substr(second_null + 1);

    return {username, password};
}

std::string CommandHandler::hashPassword(const std::string& password)
{
    std::string hashed_password;
    std::vector<unsigned char> hash(crypto_pwhash_STRBYTES);

    if (crypto_pwhash_str(reinterpret_cast<char*>(hash.data()), password.c_str(),
                          password.size(), crypto_pwhash_OPSLIMIT_INTERACTIVE,
                          crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
        throw std::runtime_error("Password hashing failed");
                          }

    hashed_password.assign(hash.begin(), hash.end());
    return hashed_password;
}