#ifndef SSLSOCKETMANAGER_H
#define SSLSOCKETMANAGER_H

#include <memory>
#include <future>
#include <string>

#include <boost/asio/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>

using TlsSocket = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;
using TlsSocketPtr = std::shared_ptr<TlsSocket>;

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
