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
	explicit SocketWrapper(std::shared_ptr<TcpSocket> tcp_socket)
		: socket_(tcp_socket) {}

	explicit SocketWrapper(std::shared_ptr<SslSocket> ssl_socket)
		: socket_(ssl_socket) {}

	[[nodiscard]] bool is_tls() const {
		return std::holds_alternative<std::shared_ptr<SslSocket>>(socket_);
	}

	template <typename SocketType>
	SocketType* get() {
		//std::cout << "Attempting to get " << typeid(SocketType).name()
		//		  << std::endl;
		if (std::holds_alternative<std::shared_ptr<SocketType>>(socket_)) {
			return std::get<std::shared_ptr<SocketType>>(socket_).get();
		} else {
			std::cerr << "Error: Incorrect socket type requested." << std::endl;
			return nullptr;
		}
	}

private:
	std::variant<std::shared_ptr<TcpSocket>, std::shared_ptr<SslSocket>>
		socket_;
};

#endif	// SOCKETWRAPPER_H
