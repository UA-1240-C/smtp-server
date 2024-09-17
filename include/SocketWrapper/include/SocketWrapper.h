#pragma once

#ifndef SOCKETWRAPPER_H
#define SOCKETWRAPPER_H

#include <future>
#include <memory>
#include <variant>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "TcpSocketManager.h"
#include "TlsSocketManager.h"
#include "Logger.h"

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
    explicit SocketWrapper(const TcpSocketPtr& tcp_socket);

    /**
     * @brief Constructs a SocketWrapper for an SSL/TLS socket.
     * @param ssl_socket A shared pointer to the SSL/TLS socket.
     */
    explicit SocketWrapper(const TlsSocketPtr& ssl_socket);

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
    void set_socket(SocketType socket) {
        Logger::LogDebug(typeid(socket).name());
        if constexpr (std::is_same_v<SocketType, TcpSocketPtr>) {
            m_socket_wrapper = std::make_shared<TcpSocketManager>(socket);
        } else if constexpr (std::is_same_v<SocketType, TlsSocketPtr>) {
            m_socket_wrapper = std::make_shared<TlsSocketManager>(socket);
        } else {
            Logger::LogDebug(typeid(socket).name());
            Logger::LogDebug("Unsupported socket type");
        }
    }

    /**
     * @brief Retrieves a pointer to the TcpSocketManager if it exists in the socket wrapper.
     *
     * This function attempts to extract a shared pointer to the TcpSocketManager from the `m_socket_wrapper` variant.
     * If successful, it returns a raw pointer to the TcpSocketManager; otherwise, it returns nullptr.
     *
     * @return A pointer to TcpSocketManager if present, or nullptr if not.
     *
     * @note The function is marked as [[nodiscard]], indicating that the return value should not be ignored.
     */
    [[nodiscard]] TcpSocketManager* get_tcp_socket_manager();

    /**
     * @brief Retrieves a constant pointer to the TcpSocketManager if it exists in the socket wrapper.
     *
     * This function is a const-qualified version that returns a constant pointer to the TcpSocketManager.
     * It attempts to extract a shared pointer from the `m_socket_wrapper`. If successful, it returns the raw constant pointer.
     *
     * @return A constant pointer to TcpSocketManager if present, or nullptr if not.
     *
     * @note The function is marked as [[nodiscard]], indicating that the return value should not be ignored.
     */
    [[nodiscard]] const TcpSocketManager* get_tcp_socket_manager() const;

    /**
     * @brief Retrieves a pointer to the TlsSocketManager if it exists in the socket wrapper.
     *
     * This function attempts to extract a shared pointer to the TlsSocketManager from the `m_socket_wrapper` variant.
     * If successful, it returns a raw pointer to the TlsSocketManager; otherwise, it returns nullptr.
     *
     * @return A pointer to TlsSocketManager if present, or nullptr if not.
     *
     * @note The function is marked as [[nodiscard]], indicating that the return value should not be ignored.
     */
    [[nodiscard]] TlsSocketManager* get_tls_socket_manager();

    /**
     * @brief Retrieves a constant pointer to the TlsSocketManager if it exists in the socket wrapper.
     *
     * This function is a const-qualified version that returns a constant pointer to the TlsSocketManager.
     * It attempts to extract a shared pointer from the `m_socket_wrapper`. If successful, it returns the raw constant pointer.
     *
     * @return A constant pointer to TlsSocketManager if present, or nullptr if not.
     *
     * @note The function is marked as [[nodiscard]], indicating that the return value should not be ignored.
     */
    [[nodiscard]] const TlsSocketManager* get_tls_socket_manager() const;
	    
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
    std::future<void> WriteAsync(const std::string& message);

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
    std::future<std::string> ReadAsync(size_t max_length) const;

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
    std::future<void> PerformTlsHandshake(boost::asio::ssl::stream_base::handshake_type handshake_type, boost::asio::ssl::context& context);

    /**
     * @brief Asynchronously resolves a hostname and service and attempts to connect to the resolved address.
     *
     * This function performs asynchronous hostname DNS resolution and connects to the first available result.
     * It checks whether the `m_socket_wrapper` holds a `TcpSocketManager` and uses its socket for the connection.
     *
     * If the resolution or connection fails, the promise is set with an exception.
     * Otherwise, it sets the promise's value upon successful connection.
     *
     * @param io_context The Boost.Asio I/O context used for managing asynchronous operations.
     * @param hostname The hostname of the server to resolve (e.g., "smtp.gmail.com").
     * @param service The service or port number to resolve (e.g., "587").
     *
     * @return A `std::future<void>` that becomes ready when the connection is successfully established or an exception is thrown.
     *
     * @exception std::runtime_error Thrown if the hostname resolution or connection fails, containing the error message.
     *
     * @note The function logs the entry and exit points, as well as any errors encountered during resolution or connection.
     */
    std::future<void> ResolveAndConnectAsync(boost::asio::io_context& io_context,
                                             const std::string& hostname,
                                             const std::string& service);

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
    std::variant<std::shared_ptr<TcpSocketManager>, std::shared_ptr<TlsSocketManager>> m_socket_wrapper;  ///< The variant holding either a TCP or SSL/TLS socket.
    std::shared_ptr<boost::asio::steady_timer>
        m_timeout_timer;  ///< Timer to count down until the client is sent to timeout
};
}  // namespace ISXSocketWrapper

#endif  // SOCKETWRAPPER_H
