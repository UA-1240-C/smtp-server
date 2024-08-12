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
		if (!std::holds_alternative<std::shared_ptr<SocketType>>(m_socket)) {
			std::cerr << "Error: Incorrect socket type requested." << std::endl;
			return nullptr;
		}
		return std::get<std::shared_ptr<SocketType>>(m_socket).get();
	}

	std::future<void> sendResponseAsync(const std::string& message);
	std::future<std::string> readFromSocketAsync(size_t max_length);
	std::future<void> startTlsAsync(boost::asio::ssl::context& context);

    void Close();

	bool IsOpen() const;
	
private:
	std::variant<std::shared_ptr<TcpSocket>, std::shared_ptr<SslSocket>> m_socket;
	bool m_is_tls;

	void CloseTcp();
	void CloseSsl();
};

#endif  // SOCKETWRAPPER_H
