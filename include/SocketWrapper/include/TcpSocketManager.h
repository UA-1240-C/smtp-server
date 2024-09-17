#ifndef TCPSOCKETMANAGER_H
#define TCPSOCKETMANAGER_H

#include <memory>
#include <future>
#include <string>

#include <boost/asio.hpp>

using TcpSocket = boost::asio::ip::tcp::socket;
using TcpSocketPtr = std::shared_ptr<TcpSocket>;

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
