#pragma once

#ifndef SOCKETWRAPPER_H
#define SOCKETWRAPPER_H

#include <iostream>
#include <memory>
#include <variant>
#include <future>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

using TcpSocket = boost::asio::ip::tcp::socket;
using SslSocket = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;

namespace ISXSocketWrapper
{
/**
 * @class SocketWrapper
 * @brief A class that wraps TCP and SSL/TLS sockets for asynchronous operations.
 *
 * This class provides an abstraction for handling both plain TCP sockets and
 * SSL/TLS encrypted sockets. It supports operations such as sending responses,
 * reading from the socket, and initiating an SSL/TLS handshake.
 */
class SocketWrapper
{
public:
 /**
  * @brief Constructs a SocketWrapper for a TCP socket.
  * @param tcp_socket A shared pointer to the TCP socket.
  */
 explicit SocketWrapper(std::shared_ptr<TcpSocket> tcp_socket);

 /**
  * @brief Constructs a SocketWrapper for an SSL/TLS socket.
  * @param ssl_socket A shared pointer to the SSL/TLS socket.
  */
 explicit SocketWrapper(std::shared_ptr<SslSocket> ssl_socket);

 /**
  * @brief Checks if the socket is an SSL/TLS socket.
  * @return True if the socket is an SSL/TLS socket, false otherwise.
  */
 [[nodiscard]] bool IsTls() const;

 /**
  * @brief Gets the underlying socket of a specific type.
  * @tparam SocketType The type of socket to retrieve (TcpSocket or SslSocket).
  * @return A pointer to the socket of the requested type, or nullptr if the type is incorrect.
  */
 template <typename SocketType>
 SocketType* get_socket()
 {
  if (!std::holds_alternative<std::shared_ptr<SocketType>>(m_socket))
  {
   std::cerr << "Error: Incorrect socket type requested." << std::endl;
   return nullptr;
  }
  return std::get<std::shared_ptr<SocketType>>(m_socket).get();
 }

 /**
  * @brief Sends a response asynchronously.
  * @param message The message to send.
  * @return A future that will be set when the send operation is complete.
  */
 std::future<void> SendResponseAsync(const std::string& message);

 /**
  * @brief Reads data from the socket asynchronously.
  * @param max_length The maximum number of bytes to read.
  * @return A future that contains the read data when the operation is complete.
  */
 std::future<std::string> ReadFromSocketAsync(size_t max_length);

 /**
  * @brief Starts an SSL/TLS handshake asynchronously.
  * @param context The SSL/TLS context to use for the handshake.
  * @return A future that will be set when the handshake is complete.
  */
 std::future<void> StartTlsAsync(boost::asio::ssl::context& context);

private:
 std::variant<std::shared_ptr<TcpSocket>, std::shared_ptr<SslSocket>> m_socket; ///< The variant holding either a TCP or SSL/TLS socket.
 bool m_is_tls; ///< Flag indicating whether the socket is an SSL/TLS socket.
};
}

#endif  // SOCKETWRAPPER_H
