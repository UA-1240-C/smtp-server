#pragma once

#ifndef SOCKETWRAPPER_H
#define SOCKETWRAPPER_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <future>
#include <iostream>
#include <memory>
#include <variant>

#include "Logger.h"

using TcpSocket = boost::asio::ip::tcp::socket;
using TcpSocketPtr = std::shared_ptr<TcpSocket>;
using SslSocket = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;
using SslSocketPtr = std::shared_ptr<SslSocket>;

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

    template <typename SocketType>
    void set_socket(std::shared_ptr<SocketType> socket)
    {
        m_socket = socket;
        m_is_tls = std::is_same_v<SocketType, SslSocket>;
        Logger::LogDebug("SocketWrapper::set_socket: Socket type set to " +
                         std::string(m_is_tls ? "SslSocket" : "TcpSocket"));
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

    /**
     * @brief Closes the socket.
     *
     * This method closes the socket, which could be either a TCP or SSL socket.
     * It determines the type of socket currently held by the `SocketWrapper`
     * and calls the appropriate method to close it.
     */
    void Close();

    /**
     * @brief Checks if the socket is open.
     *
     * This method returns whether the socket is open. It checks the type of socket
     * (TCP or SSL) and verifies the state of the socket accordingly.
     *
     * @return True if the socket is open, false otherwise.
     */
    bool IsOpen() const;

    /**
     * @brief Sets the timeout timer for the socket.
     *
     * This function sets the internal timeout timer for the socket. The timer
     * can be used later to start, reset, or cancel timeout operations.
     *
     * @param timeout_timer Shared pointer to a steady timer used for managing timeouts.
     */
    void SetTimeoutTimer(std::shared_ptr<boost::asio::steady_timer> timeout_timer);

    /**
     * @brief Starts the timeout timer for the socket.
     *
     * This function starts the timeout timer with the specified duration. If the
     * timer expires, the connection associated with this socket will be closed.
     *
     * @param timeout_duration The duration after which the timeout should occur.
     */
    void StartTimeoutTimer(std::chrono::seconds timeout_duration);

    /**
     * @brief Cancels the currently running timeout timer.
     *
     * This function cancels the current timeout timer. If the timer was not set
     * or an error occurs during cancellation, an error message will be logged.
     */
    void CancelTimeoutTimer();

private:
    std::variant<TcpSocketPtr, SslSocketPtr> m_socket;   ///< The variant holding either a TCP or SSL/TLS socket.
    bool m_is_tls;  ///< Flag indicating whether the socket is an SSL/TLS socket.
    std::shared_ptr<boost::asio::steady_timer> m_timeout_timer; ///< Timer to count down until the client is sent to timeout


private:
    /**
    * @brief Closes a TCP socket.
    *
    * This method closes the socket, which could be only TCP socket.
    */
    void TerminateTcpConnection(TcpSocket& tcp_socket);

    /**
    * @brief Closes a SSL socket.
    *
    * This method closes the socket, which could be only SSL socket.
    */
    void TerminateSslConnection(SslSocket& ssl_socket);

private:
    /**
     * @brief Closes a TCP socket.
     *
     * This method shuts down and then closes a TCP socket. It handles any errors
     * that occur during the shutdown and closing process.
     */
    void CloseTcp();

    /**
     * @brief Closes an SSL socket.
     *
     * This method shuts down and then closes an SSL socket. It handles any errors
     * that occur during the shutdown and closing process.
     */
    void CloseSsl();
};
}  // namespace ISXSocketWrapper

#endif  // SOCKETWRAPPER_H
