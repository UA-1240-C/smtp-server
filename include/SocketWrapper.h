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

		if (std::holds_alternative<std::shared_ptr<SocketType>>(socket_)) {
			return std::get<std::shared_ptr<SocketType>>(socket_).get();
		} else {
			std::cerr << "Error: Incorrect socket type requested." << std::endl;
			return nullptr;
		}
	}


	void upgradeToTls(boost::asio::ssl::context& ssl_context) {
		if (is_tls()) {
			std::cerr << "Socket is already using TLS." << std::endl;
			return;
		}

		auto tcp_socket = get<TcpSocket>();
		auto ssl_socket = std::make_shared<SslSocket>(std::move(*tcp_socket), ssl_context);
		socket_ = ssl_socket;

		auto ssl_stream = get<SslSocket>();
		ssl_stream->async_handshake(boost::asio::ssl::stream_base::server,
				[ssl_stream](const boost::system::error_code& error) {
						if (!error) {
								std::cout << "TLS handshake completed." << std::endl;
						} else {
								std::cerr << "TLS handshake error: " << error.message() << std::endl;
						}
				});
	}

private:
	std::variant<std::shared_ptr<TcpSocket>, std::shared_ptr<SslSocket>> socket_;
};

#endif  // SOCKETWRAPPER_H
