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
	explicit TcpSocketManager(TcpSocketPtr tcp_socket);

	std::future<void> WriteAsync(const std::string& message) const;
	std::future<std::string> ReadAsync(size_t max_length) const;
	void TerminateConnection() const;
	[[nodiscard]] bool IsOpen() const;

	TcpSocket& get_socket();
	[[nodiscard]] const TcpSocket& get_socket() const;

private:
	TcpSocketPtr m_socket;
};

#endif //TCPSOCKETMANAGER_H
