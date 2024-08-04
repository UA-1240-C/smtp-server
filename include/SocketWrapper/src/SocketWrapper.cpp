#include "SocketWrapper.h"

explicit SocketWrapper::SocketWrapper(std::shared_ptr<TcpSocket> tcp_socket)
      : socket_(tcp_socket), is_tls_(false) {}

explicit SocketWrapper::SocketWrapper(std::shared_ptr<SslSocket> ssl_socket)
    : socket_(ssl_socket), is_tls_(true) {}

[[nodiscard]] bool SocketWrapper::is_tls() const {
  return std::holds_alternative<std::shared_ptr<SslSocket>>(socket_);
}

void SocketWrapper::sendResponse(const std::string& message) {
  std::cout << "Sending response: " << message << std::endl;
  if (auto tcp_socket = get<TcpSocket>()) {
    boost::asio::write(*tcp_socket, boost::asio::buffer(message));
  } else if (auto ssl_socket = get<SslSocket>()) {
    boost::asio::write(*ssl_socket, boost::asio::buffer(message));
  } else {
    std::cerr << "Error: No valid socket available for sending data." << std::endl;
  }
}

void SocketWrapper::start_tls(boost::asio::ssl::context& context) {
  auto tcp_socket = get<TcpSocket>();
  auto ssl_socket = std::make_shared<SslSocket>(std::move(*tcp_socket), context);
  ssl_socket->set_verify_mode(boost::asio::ssl::verify_none);
  ssl_socket->async_handshake(boost::asio::ssl::stream_base::server,
      [this, ssl_socket](const boost::system::error_code& error) {
          if (!error) {
              // Successfully established TLS connection
              socket_ = ssl_socket;
              std::cout << "STARTTLS handshake successful" << std::endl;
          } else {
              std::cerr << "STARTTLS handshake error: " << error.message() << std::endl;
          }
      });
}