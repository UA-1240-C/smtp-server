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
	explicit TlsSocketManager(TlsSocketPtr ssl_socket);

	[[nodiscard]] std::future<void> WriteAsync(const std::string& message) const;
	[[nodiscard]] std::future<std::string> ReadAsync(size_t max_length) const;
	[[nodiscard]] std::future<void> PerformTlsHandshake(boost::asio::ssl::stream_base::handshake_type handshake_type) const;
	void TerminateConnection() const;
	[[nodiscard]] bool IsOpen() const;

	TlsSocket& get_socket();
	[[nodiscard]] const TlsSocket& get_socket() const;

private:
	TlsSocketPtr m_socket;
};

#endif //SSLSOCKETMANAGER_H
