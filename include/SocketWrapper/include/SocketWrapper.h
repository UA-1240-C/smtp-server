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
		if (std::holds_alternative<std::shared_ptr<SocketType>>(socket_)) {
			return std::get<std::shared_ptr<SocketType>>(socket_).get();
		} else {
			std::cerr << "Error: Incorrect socket type requested." << std::endl;
			return nullptr;
		}
	}

	void sendResponse(const std::string& message);

	void start_tls(boost::asio::ssl::context& context);

	/*std::future<void> asyncSend(const std::string& message) {
		auto promise = std::make_shared<std::promise<void>>();
		auto future = promise->get_future();

		if (auto tcp_socket = get<TcpSocket>()) {
			asyncSend(*tcp_socket, message, promise);
		} else if (auto ssl_socket = get<SslSocket>()) {
			asyncSend(*ssl_socket, message, promise);
		} else {
			promise->set_exception(std::make_exception_ptr(std::runtime_error("No valid socket available for sending data.")));
		}

		return future;
	}*/
private:
	/*template <typename AsyncWriteStream>
		void asyncSend(AsyncWriteStream& stream, const std::string& message, std::shared_ptr<std::promise<void>> promise) {
		boost::asio::async_write(stream, boost::asio::buffer(message),
				[promise](const boost::system::error_code& error, std::size_t) {
						if (error) {
								promise->set_exception(std::make_exception_ptr(boost::system::system_error(error)));
						} else {
								promise->set_value();
						}
				});
	}*/
	std::variant<std::shared_ptr<TcpSocket>, std::shared_ptr<SslSocket>> socket_;
	bool is_tls_;
};

#endif  // SOCKETWRAPPER_H
