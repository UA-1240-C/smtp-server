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

	std::future<void> SendResponseAsync(const std::string& message);
	std::future<std::string> ReadFromSocketAsync(size_t max_length);
	std::future<void> StartTlsAsync(boost::asio::ssl::context& context);

    void close();

	bool is_open() const;
	
private:
	std::variant<std::shared_ptr<TcpSocket>, std::shared_ptr<SslSocket>> socket_;
	bool is_tls_;

	void closeTcp();
	void closeSsl();
};

#endif  // SOCKETWRAPPER_H
