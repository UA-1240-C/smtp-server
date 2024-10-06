#include <exception>
#include <iostream>

#include <boost/asio.hpp>
#include <utility>

#include "TcpSocketManager.h"
#include <Logger.h>


TcpSocketManager::TcpSocketManager(TcpSocketPtr tcp_socket)
    : m_socket(std::move(tcp_socket))
{
    Logger::LogDebug("Entering TcpSocketManager::TcpSocketManager");
    Logger::LogTrace("Constructor params: TcpSocketPtr");
    Logger::LogDebug("Exiting TcpSocketManager::TcpSocketManager");
}

std::future<void> TcpSocketManager::WriteAsync(const std::string& message) const
{
    Logger::LogDebug("Entering TcpSocketManager::WriteAsync");
    Logger::LogTrace("TcpSocketManager::WriteAsync params: const std::string&");

    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();

    async_write(*m_socket, boost::asio::buffer(message),
        [promise](const boost::system::error_code& error, std::size_t) {
            if (error) {
                promise->set_exception(std::make_exception_ptr(std::runtime_error(error.message())));
            } else {
                promise->set_value();
            }
        });

    Logger::LogDebug("Exiting TcpSocketManager::WriteAsync");
    Logger::LogTrace("TcpSocketManager::WriteAsync return value: std::future<void>");
    return future;
}

std::future<std::string> TcpSocketManager::ReadAsync(size_t max_length) const
{
    Logger::LogDebug("Entering TcpSocketManager::ReadAsync");
    Logger::LogTrace("TcpSocketManager::ReadAsync params: size_t");

    auto promise = std::make_shared<std::promise<std::string>>();
    auto future = promise->get_future();
    auto buffer = std::make_shared<std::string>(max_length, '\0');

    m_socket->async_read_some(boost::asio::buffer(*buffer),
        [promise, buffer](const boost::system::error_code& error, const std::size_t length) {
            if (error) {
                promise->set_exception(std::make_exception_ptr(std::runtime_error(error.message())));
            } else {
                buffer->resize(length);
                promise->set_value(*buffer);
            }
        });

    Logger::LogDebug("Exiting TcpSocketManager::ReadAsync");
    Logger::LogTrace("TcpSocketManager::ReadAsync return value: std::future<std::string>");
    return future;
}

void TcpSocketManager::TerminateConnection() const
{
    Logger::LogDebug("Entering TcpSocketManager::TerminateConnection");

    if (m_socket->is_open()) {
        boost::system::error_code ec;
        m_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        if (ec) {
            Logger::LogError("An error occurred while shutting down a TCP socket: "
                + ec.message());
        }
        m_socket->close(ec);
        if (ec) {
            Logger::LogError("An error occurred while closing: "
                            + ec.message());        }
    }
    Logger::LogDebug("Exiting TcpSocketManager::TerminateConnection");
}

bool TcpSocketManager::IsOpen() const
{
    Logger::LogDebug("Entering TcpSocketManager::IsOpen");

    Logger::LogDebug("Exiting TcpSocketManager::IsOpen");
    return m_socket->is_open();
}

TcpSocket& TcpSocketManager::get_socket()
{
    return *m_socket;
}

const TcpSocket& TcpSocketManager::get_socket() const
{
    return *m_socket;
}