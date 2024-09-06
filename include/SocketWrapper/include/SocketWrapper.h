#pragma once

#ifndef SOCKETWRAPPER_H
#define SOCKETWRAPPER_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <future>
#include <memory>
#include <variant>

#include "Logger.h"

using TcpSocket = boost::asio::ip::tcp::socket;
using TcpSocketPtr = std::shared_ptr<TcpSocket>;
using SslSocket = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;
using SslSocketPtr = std::shared_ptr<SslSocket>;

constexpr size_t MAX_LENGTH = 1024 * 1024;

namespace ISXSocketWrapper
{
/**
 * @class SocketWrapper
 * @brief A class that wraps TCP and SSL/TLS sockets for I/O operations.
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
    explicit SocketWrapper(TcpSocketPtr tcp_socket);

    /**
     * @brief Constructs a SocketWrapper for an SSL/TLS socket.
     * @param ssl_socket A shared pointer to the SSL/TLS socket.
     */
    explicit SocketWrapper(SslSocketPtr ssl_socket);

    /**
     * @brief Checks if the socket is an SSL/TLS socket.
     * @return True if the socket is an SSL/TLS socket, false otherwise.
     */
    [[nodiscard]] bool IsTls() const;

    /**
     * @brief Sets the socket for the `SocketWrapper` and determines if it is an SSL socket.
     *
     * @tparam SocketType The type of the socket being set. This can be a `TcpSocket` or `SslSocket`.
     *
     * @param socket The socket object to be assigned to the internal socket member.
     *
     * @details This function sets the internal socket member to the provided `socket`.
     * It also determines if the provided socket is of type `SslSocket` and sets
     * the `m_is_tls` flag accordingly. The function logs the type of socket being set
     * (either "SslSocket" or "TcpSocket") for debugging purposes.
     */
    template <typename SocketType>
    void set_socket(SocketType socket)
    {
     Logger::LogDebug("Entering SocketWrapper::set_socket.");
     Logger::LogTrace("SocketWrapper::set_socket parameters: SocketType");
        m_socket = socket;
        m_is_tls = std::is_same_v<SocketType, SslSocket>;
        Logger::LogDebug("SocketWrapper::set_socket: Socket type set to " +
                         std::string(m_is_tls ? "SslSocket" : "TcpSocket"));
    }

    std::future<void> Connect(const std::string& host, unsigned short port);
    std::future<void> PerformTlsHandshake(boost::asio::ssl::context& context, boost::asio::ssl::stream_base::handshake_type type);

    /**
     * @brief Asynchronously sends a response message over a socket.
     *
     * @param message The message to be sent as a `std::string`.
     * @return A `std::future<void>` that can be used to wait for the send operation to complete.
     *
     * @details This function asynchronously sends the provided message over the socket
     * wrapped by the `SocketWrapper`. The socket can be either a TCP or an SSL/TLS socket.
     *
     * The function determines the type of socket and performs an asynchronous write operation
     * using the appropriate `async_write_some` function. If the operation succeeds, the promise
     * associated with the future is set to `void`. If an error occurs, the promise is set
     * with an exception, and the error is logged.
     *
     * @note The function returns immediately with a future that will be set once the
     * asynchronous operation completes.
     */
    std::future<void> SendResponseAsync(const std::string& message);

    /**
     * @return A future that contains the read data when the operation is complete.
    /**
     * @brief Asynchronously sends a response message over a socket.
     *
     * @param max_length The maximum number of bytes to read.
     * @return A std::future<std::string> that contains the read data when the operation is complete.
     *
     * @details This function asynchronously reads the provided number if bytes over the socket
     * wrapped by the `SocketWrapper`. The socket can be either a TCP or an SSL/TLS socket.
     *
     * The function determines the type of socket and performs an asynchronous read operation
     * using the appropriate `async_read_some` function. If the operation succeeds, the promise
     * associated with the future is set to `std::string`. If an error occurs, the promise is set
     * with an exception, and the error is logged.
     *
     * @note The function returns immediately with a future that will be set once the
     * asynchronous operation completes.
     */
    std::future<std::string> ReadFromSocketAsync(size_t max_length);

     /**
     * @brief Asynchronously initiates a TLS handshake, upgrading a TCP socket to an SSL/TLS socket.
     *
     * @param context Reference to a `boost::asio::ssl::context` that contains the SSL context configuration.
     * @return A `std::future<void>` that will be set once the handshake operation completes.
     *
     * @details This function attempts to upgrade an existing TCP socket managed by the `SocketWrapper` to an SSL/TLS socket.
     *
     * It first retrieves the `TcpSocket` from the internal socket variant. If successful, it creates a new `SslSocket`
     * using the provided SSL context. The function then initiates an asynchronous SSL/TLS handshake as a server.
     *
     * If the handshake succeeds, the internal socket is replaced with the newly created `SslSocket`, and the associated
     * promise is fulfilled. In the event of an error during the handshake, the promise is set with an exception,
     * and the error is logged.
     *
     * @note The function returns immediately with a `std::future<void>` that will be set once the handshake completes,
     * allowing the caller to wait for the completion if desired.
     */
    std::future<void> StartTlsAsync(boost::asio::ssl::context& context);

    /**
     * @brief Closes the socket.
     *
     * This method closes the socket, which could be either a TCP or SSL socket.
     * It determines the type of socket currently held by the `SocketWrapper`
     * and calls the appropriate private method to close it.
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
    [[nodiscard]] bool IsOpen() const;
public:
    /**
    * @brief Sets the timeout timer for the socket.
    *
    * This function assigns a `boost::asio::steady_timer` to the internal timeout timer member.
    * The timer will be used to manage timeout for socket connection.
    *
    * @param timeout_timer A shared pointer to a `boost::asio::steady_timer` object.
    */
    void set_timeout_timer(std::shared_ptr<boost::asio::steady_timer> timeout_timer);

    /**
     * @brief Starts the timeout timer for the socket.
     *
     * This function sets the timeout duration for the socket and starts the timer. If the timer expires
     * without being reset, the socket will be closed, and a warning will be logged.
     *
     * @param timeout_duration The duration after which the timeout will trigger, expressed in seconds.
     */
    void StartTimeoutTimer(std::chrono::seconds timeout_duration);

    /**
     * @brief Cancels the timeout timer for the socket.
     *
     * This function attempts to cancel the currently active timeout timer. If successful, a
     * confirmation is logged. If the timer is null or an error occurs during cancellation, an error is logged.
     */
    void CancelTimeoutTimer();
private:
    /**
     * @brief Closes a TCP socket.
     *
     * This method closes the socket, which could be only TCP socket.
     */
    void TerminateTcpConnection(TcpSocket& tcp_socket);

    /**
     * @brief Closes an SSL socket.
     *
     * This method closes the socket, which could be only SSL socket.
     */
    void TerminateSslConnection(SslSocket& ssl_socket);
private:
    std::variant<TcpSocketPtr, SslSocketPtr> m_socket;  ///< The variant holding either a TCP or SSL/TLS socket.
    bool m_is_tls;                                      ///< Flag indicating whether the socket is an SSL/TLS socket.
    std::shared_ptr<boost::asio::steady_timer>
        m_timeout_timer;  ///< Timer to count down until the client is sent to timeout
};
}  // namespace ISXSocketWrapper

#endif  // SOCKETWRAPPER_H
