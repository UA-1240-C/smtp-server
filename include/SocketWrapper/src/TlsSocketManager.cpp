#include <exception>
#include <iostream>

#include <boost/asio.hpp>
#include <utility>

#include "TlsSocketManager.h"
#include "Logger.h"

TlsSocketManager::TlsSocketManager(TlsSocketPtr ssl_socket)
    : m_socket(std::move(ssl_socket))
{
    Logger::LogDebug("Entering TlsSocketManager::TlsSocketManager");
    Logger::LogTrace("Constructor params: TlsSocketPtr");
    Logger::LogDebug("Exiting TlsSocketManager::TlsSocketManager");
}

std::future<void> TlsSocketManager::WriteAsync(const std::string& message) const
{
    Logger::LogDebug("Entering TlsSocketManager::WriteAsync");
    Logger::LogTrace("TlsSocketManager::WriteAsync params: const std::string&");

    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();

    boost::asio::async_write(*m_socket, boost::asio::buffer(message),
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

std::future<std::string> TlsSocketManager::ReadAsync(size_t max_length) const
{
    Logger::LogDebug("Entering TlsSocketManager::ReadAsync");
    Logger::LogTrace("TlsSocketManager::ReadAsync params: size_t");

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
    Logger::LogDebug("Exiting TlsSocketManager::ReadAsync");
    Logger::LogTrace("TlsSocketManager::ReadAsync return value: std::future<std::string>");

    return future;
}

std::future<void> TlsSocketManager::PerformTlsHandshake(boost::asio::ssl::stream_base::handshake_type handshake_type) const
{
    Logger::LogDebug("Entering TlsSocketManager::PerformTlsHandshake");
    Logger::LogTrace("TlsSocketManager::PerformTlsHandshake params: boost::asio::ssl::stream_base::handshake_type");

    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();
    
    m_socket->async_handshake(handshake_type,
        [promise](const boost::system::error_code& error) {
            if (error) {
                promise->set_exception(std::make_exception_ptr(std::runtime_error(error.message())));
            } else {
                
                promise->set_value();
            }
        });

    Logger::LogDebug("Exitiing TlsSocketManager::PerformTlsHandshake");
    Logger::LogTrace("TlsSocketManager::PerformTlsHandshake return value: std::future<void>");
    return future;
}

void TlsSocketManager::TerminateConnection() const
{
    Logger::LogDebug("Entering TlsSocketManager::TerminateConnection");

    if (m_socket->lowest_layer().is_open()) {
        boost::system::error_code ec;
        m_socket->lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        if (ec) {
            Logger::LogDebug("An error occured while shutting down SSL socket: "
                             + ec.message());
        }
        m_socket->lowest_layer().close(ec);
        if (ec) {
            Logger::LogDebug("An error occured while closing SSL socket: "
                                         + ec.message());        }
    }
    Logger::LogDebug("Exitiing TlsSocketManager::TerminateConnection");
}

bool TlsSocketManager::IsOpen() const {
    Logger::LogDebug("Entering TlsSocketManager::IsOpen");

    Logger::LogDebug("Exiting TlsSocketManager::IsOpen");
    return m_socket->lowest_layer().is_open();
}

TlsSocket& TlsSocketManager::get_socket()
{
    return *m_socket;
}

const TlsSocket& TlsSocketManager::get_socket() const
{
    return *m_socket;
}
