#ifndef TCPSOCKETMANAGER_H
#define TCPSOCKETMANAGER_H

#include <memory>
#include <future>
#include <string>

#include <boost/asio.hpp>

using TcpSocket = boost::asio::ip::tcp::socket;
using TcpSocketPtr = std::shared_ptr<TcpSocket>;

/**
 * @class TcpSocketManager
 * @brief Manages asynchronous TCP socket operations.
 *
 * The `TcpSocketManager` class provides an abstraction for managing a TCP socket with asynchronous
 * read and write operations using Boost.Asio. It encapsulates a raw TCP socket and provides methods
 * to perform asynchronous reads, writes, and to manage the socket's lifecycle.
 *
 * This class allows for non-blocking communication over TCP, enabling the handling of socket
 * operations in an asynchronous manner. It supports basic operations such as sending and receiving
 * messages and managing socket connections.
 *
 * Key functionalities include:
 * - Performing asynchronous writes to the socket (`WriteAsync`).
 * - Performing asynchronous reads from the socket (`ReadAsync`).
 * - Terminating the socket connection and handling any potential errors (`TerminateConnection`).
 * - Checking if the socket is open (`IsOpen`).
 * - Accessing the underlying raw TCP socket (`get_socket`).
 *
 * @note The `TcpSocketManager` assumes that the underlying socket is properly initialized and
 *       connected before performing any read or write operations. Ensure to handle any potential
 *       exceptions that may arise from asynchronous operations.
 *
 * Example usage:
 * @code
 * auto tcp_socket = std::make_shared<boost::asio::ip::tcp::socket>(io_context);
 * TcpSocketManager tcp_manager(tcp_socket);
 * auto write_future = tcp_manager.WriteAsync("Hello, World!");
 * write_future.get(); // Wait for write to complete or throw an exception
 * auto read_future = tcp_manager.ReadAsync(1024);
 * std::string message = read_future.get(); // Wait for read to complete or throw an exception
 * @endcode
 */
class TcpSocketManager {
public:
	/**
	 * @brief Constructor for TcpSocketManager that initializes the socket manager with a TCP socket.
 	 *
	 * @param tcp_socket A smart pointer to a TCP socket (`TcpSocketPtr`) used for communication.
	 */
	explicit TcpSocketManager(TcpSocketPtr tcp_socket);

	/**
	 * @brief Asynchronously writes a message to the TCP socket.
	 * 
	 * This function sends data through the TCP socket asynchronously and sets a promise based on the success or failure
	 * of the operation.
	 * 
	 * @param message The message to be sent over the TCP socket.
	 * 
	 * @return A `std::future<void>` which indicates completion of the write operation.
	 *         If the operation fails, the future will contain an exception.
	 */
	[[nodiscard]] std::future<void> WriteAsync(const std::string& message) const;
	
	/**
	 * @brief Asynchronously reads data from the TCP socket.
	 * 
	 * This function reads data from the TCP socket asynchronously and sets a promise with the received data or an error.
	 * 
	 * @param max_length The maximum length of data to read.
	 * 
	 * @return A `std::future<std::string>` which will contain the data read from the socket.
	 * If the operation fails, the future will contain an exception.
	 */
	[[nodiscard]] std::future<std::string> ReadAsync(size_t max_length) const;
	
	/**
	 * @brief Terminates the connection by shutting down and closing the TCP socket.
	 * 
	 * This function ensures that the TCP socket is properly shutdown and closed.
	 * It logs any errors that occur during the shutdown and close operations.
	 */
	void TerminateConnection() const;
	
	/**
	 * @brief Checks whether the TCP socket is open.
	 * 
	 * @return `true` if the socket is open, otherwise `false`.
	 */
	[[nodiscard]] bool IsOpen() const;

	/**
	 * @brief Returns a reference to the underlying TCP socket.
	 *
	 * @return A reference to the managed TCP socket.
	 */
	TcpSocket& get_socket();
	
	/**
	 * @brief Returns a constant reference to the underlying TCP socket.
	 * 
	 * @return A const reference to the managed TCP socket.
	 */
	[[nodiscard]] const TcpSocket& get_socket() const;

private:
	TcpSocketPtr m_socket;
};

#endif //TCPSOCKETMANAGER_H
