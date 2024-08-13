#pragma once

#ifndef SOCKETWRAPPER_H
#define SOCKETWRAPPER_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <iostream>
#include <memory>
#include <variant>

using TcpSocket = boost::asio::ip::tcp::socket;
using SslSocket = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;

class SocketWrapper {
public:
	explicit SocketWrapper(std::shared_ptr<TcpSocket> tcp_socket);

	explicit SocketWrapper(std::shared_ptr<SslSocket> ssl_socket);

	[[nodiscard]] bool is_tls() const;

	template <typename SocketType>
	SocketType* get() {
		if (!std::holds_alternative<std::shared_ptr<SocketType>>(m_socket_)) {
			std::cerr << "Error: Incorrect socket type requested." << std::endl;
			return nullptr;
		}
		return std::get<std::shared_ptr<SocketType>>(m_socket_).get();
	}

	std::future<void> sendResponseAsync(const std::string& message);
	std::future<std::string> readFromSocketAsync(size_t max_length);
	std::future<void> startTlsAsync(boost::asio::ssl::context& context);

	void SetTimeoutTimer(std::shared_ptr<boost::asio::steady_timer> timeout_timer);
	void StartTimeoutTimer(std::chrono::seconds timeout_duration);
	void CancelTimeoutTimer();

	void Close();

	bool IsOpen() const;
	
private:
	std::variant<std::shared_ptr<TcpSocket>, std::shared_ptr<SslSocket>> m_socket_;
	bool is_tls_;

	std::shared_ptr<boost::asio::steady_timer> m_timeout_timer_;

	void CloseTcp();
	void CloseSsl();
};

#endif  // SOCKETWRAPPER_H
