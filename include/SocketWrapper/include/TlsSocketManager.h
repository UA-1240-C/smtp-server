#ifndef SSLSOCKETMANAGER_H
#define SSLSOCKETMANAGER_H

#include <memory>
#include <future>
#include <string>

#include <boost/asio/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>

using TlsSocket = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;
using TlsSocketPtr = std::shared_ptr<TlsSocket>;

/**
 * @class TlsSocketManager
 * @brief Manages an asynchronous TLS (Transport Layer Security) connection.
 *
 * The `TlsSocketManager` class encapsulates an SSL/TLS stream and provides methods to perform
 * TLS handshakes and manage the secure connection. It operates over an underlying socket that
 * supports asynchronous operations using Boost.Asio.
 *
 * This class provides an abstraction over the raw TLS operations and allows for setting up
 * a secure communication channel by performing TLS handshakes. It handles both client-side and
 * server-side handshakes, and uses futures to handle asynchronous operations and provide
 * feedback on success or failure.
 *
 * Key functionalities include:
 * - Performing asynchronous writes to the socket (`WriteAsync`).
 * - Performing asynchronous reads from the socket (`ReadAsync`).
 * - Terminating the socket connection and handling any potential errors (`TerminateConnection`).
 * - Checking if the socket is open (`IsOpen`).
 * - Accessing the underlying raw TCP socket (`get_socket`).
 * - Performing asynchronous TLS handshakes (`PerformTlsHandshake`).
 *
 * @note Ensure that the underlying socket is properly initialized and connected before performing
 *       any TLS operations. The `TlsSocketManager` assumes that the underlying socket supports
 *       asynchronous operations.
 *
 * Example usage:
 * @code
 * boost::asio::ssl::context context(boost::asio::ssl::context::tls_client);
 * auto tls_manager = std::make_shared<TlsSocketManager>(std::make_shared<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>(io_context, context));
 * auto handshake_future = tls_manager->PerformTlsHandshake(boost::asio::ssl::stream_base::client);
 * handshake_future.get(); // Wait for handshake to complete or throw an exception
 * @endcode
 */
class TlsSocketManager {
public:
	/**
	 * @brief Constructor for TlsSocketManager that initializes the socket manager with a TLS socket.
	 *
	 * @param ssl_socket A smart pointer to a TLS socket (`TlsSocketPtr`) used for communication.
	 */
	explicit TlsSocketManager(TlsSocketPtr ssl_socket);

	/**
	 * @brief Asynchronously writes a message to the TLS socket.
	 *
	 * This function sends data through the TLS socket asynchronously and sets a promise based on the success or failure
	 * of the operation.
	 *
	 * @param message The message to be sent over the TLS socket.
	 *
	 * @return A `std::future<void>` which indicates completion of the write operation.
	 *         If the operation fails, the future will contain an exception.
	 */
	[[nodiscard]] std::future<void> WriteAsync(const std::string& message) const;
	
	/**
	 * @brief Asynchronously reads data from the TLS socket.
	 * 
	 * This function reads data from the TLS socket asynchronously and sets a promise with the received data or an error.
	 * 
	 * @param max_length The maximum length of data to read.
	 * 
	 * @return A `std::future<std::string>` which will contain the data read from the socket.
	 * If the operation fails, the future will contain an exception.
	 */
	[[nodiscard]] std::future<std::string> ReadAsync(size_t max_length) const;
	
	/**
	 * @brief Asynchronously performs a TLS handshake on the underlying socket.
	 * 
	 * This function initiates an asynchronous TLS handshake operation, either for the client or server side, depending
	 * on the handshake type. The result of the handshake is returned through a future, which will either complete
	 * successfully or contain an exception in case of an error.
	 * 
	 * @param handshake_type Specifies whether the handshake is for the client or server side, using 
	 * `boost::asio::ssl::stream_base::handshake_type`.
	 * 
 	 * @return A `std::future<void>` which will complete when the handshake operation is done. 
	 *         If the handshake fails, the future will contain an exception describing the error.
	 * 
	 * @exception std::runtime_error Thrown if the handshake operation fails with a specific error.
	 */
	[[nodiscard]] std::future<void> PerformTlsHandshake(boost::asio::ssl::stream_base::handshake_type handshake_type) const;
	
	/**
	 * @brief Terminates the connection by shutting down and closing the TLS socket.
	 * 
	 * This function ensures that the TLS socket is properly shutdown and closed.
	 * It logs any errors that occur during the shutdown and close operations.
	 */
	void TerminateConnection() const;
	
	/**
	 * @brief Checks whether the TLS socket is open.
	 * 
	 * @return `true` if the socket is open, otherwise `false`.
	 */
	[[nodiscard]] bool IsOpen() const;

	/**
	 * @brief Returns a reference to the underlying TLS socket.
	 *
	 * @return A reference to the managed TLS socket.
	 */
	TlsSocket& get_socket();
	
	/**
 	 * @brief Returns a constant reference to the underlying TLS socket.
	 * 
	 * @return A constant reference to the managed TLS socket.
	 */
	[[nodiscard]] const TlsSocket& get_socket() const;

private:
	TlsSocketPtr m_socket;
};

#endif //SSLSOCKETMANAGER_H
