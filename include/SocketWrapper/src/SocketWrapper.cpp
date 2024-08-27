#include "SocketWrapper.h"

#include <utility>

namespace ISXSocketWrapper
{
SocketWrapper::SocketWrapper(TcpSocketPtr tcp_socket)
    : m_socket(tcp_socket), m_is_tls(false)
{
    Logger::LogDebug("Entering SocketWrapper TCP constructor");
    Logger::LogTrace("Constructor params: TcpSocket" );

    Logger::LogDebug("Exiting SocketWrapper TCP constructor");
}

SocketWrapper::SocketWrapper(SslSocketPtr ssl_socket)
    : m_socket(ssl_socket), m_is_tls(true)
{
    Logger::LogDebug("Entering SocketWrapper SSL constructor");
    Logger::LogTrace("Constructor params: SslSocket" );

    Logger::LogDebug("Exiting SocketWrapper SSL constructor");
}

[[nodiscard]] bool SocketWrapper::IsTls() const
{
    Logger::LogDebug("Entering SocketWrapper::IsTls");
    Logger::LogTrace("Checking if connection is TLS: " + std::to_string(m_is_tls));

    Logger::LogDebug("Exiting SocketWrapper::IsTls");
    return m_is_tls;
}

std::future<void> SocketWrapper::SendResponseAsync(const std::string& message)
{
    Logger::LogDebug("Entering SocketWrapper::SendResponseAsync");
    Logger::LogTrace("SocketWrapper::SendResponseAsync parameter: std::string reference" + message);

    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();

    if (auto* tcp_socket = std::get_if<TcpSocketPtr>(&m_socket))
    {
        Logger::LogProd("Using TCP socket for sending message.");
        async_write(**tcp_socket, boost::asio::buffer(message),
            [promise](const boost::system::error_code& error, std::size_t) {
                if (error)
                {
                    Logger::LogError("Error in async_write for TCP socket: " + error.message());
                    promise->set_exception(std::make_exception_ptr(std::runtime_error(error.message())));
                }
                else
                {
                    Logger::LogProd("Message sent successfully via TCP socket.");
                    promise->set_value();
                }
            });
    }
    else if (auto* ssl_socket = std::get_if<SslSocketPtr>(&m_socket))
    {
        Logger::LogProd("Using SSL socket for sending message.");
        async_write(**ssl_socket, boost::asio::buffer(message),
            [promise](const boost::system::error_code& error, std::size_t) {
                if (error) {
                    Logger::LogError("Error in async_write for SSL socket: " + error.message());
                    promise->set_exception(std::make_exception_ptr(std::runtime_error(error.message())));
                } else {
                    Logger::LogProd("Message sent successfully via SSL socket.");
                    promise->set_value();
                }
            });
    }
    else
    {
        const std::string error_message = "No valid socket available for sending data.";
        Logger::LogError(error_message);
        promise->set_exception(
            std::make_exception_ptr(
                std::runtime_error(error_message)));
    }

    Logger::LogTrace("SocketWrapper::SendResponseAsync returning an std::future object.");
    Logger::LogDebug("Exiting SocketWrapper::SendResponseAsync");

    return future;
}


std::future<std::string> SocketWrapper::ReadFromSocketAsync(size_t max_length)
{
    Logger::LogDebug("Entering SocketWrapper::ReadFromSocketAsync");
    Logger::LogTrace("SocketWrapper::ReadFromSocketAsync parameter: size_t " + std::to_string(max_length));

    auto promise = std::make_shared<std::promise<std::string>>();
    auto future = promise->get_future();
    auto buffer = std::make_shared<std::string>(max_length, '\0');

    if (auto* tcp_socket = std::get_if<TcpSocketPtr>(&m_socket))
    {
        Logger::LogProd("Using TCP socket for sending message.");
        (*tcp_socket)->async_read_some(boost::asio::buffer(*buffer),
            [promise, buffer](const boost::system::error_code& error, const std::size_t length)
            {
                if (error)
                {
                    Logger::LogError("Error in async_read_some for TCP socket: " + error.message());
                    promise->set_exception(std::make_exception_ptr(std::runtime_error(error.message())));
                }
                else
                {
                    Logger::LogProd("Message sent successfully via TCP socket.");
                    buffer->resize(length);
                    promise->set_value(*buffer);
                }
            });
    }
    else if (auto* ssl_socket = std::get_if<SslSocketPtr>(&m_socket))
    {
        Logger::LogProd("Using SSL socket for sending message.");
        (*ssl_socket)->async_read_some(boost::asio::buffer(*buffer),
            [promise, buffer](const boost::system::error_code& error, const std::size_t length) {
                if (error)
                {
                    Logger::LogError("Error in async_read_some for SSL socket: " + error.message());
                    promise->set_exception(
                        std::make_exception_ptr(std::runtime_error(error.message())));
                } else
                {
                    Logger::LogProd("Message sent successfully via TCP socket.");
                    buffer->resize(length);
                    promise->set_value(*buffer);
                }
            });
    }
    else
    {
        const std::string error_message = "No valid socket available for sending data.";
        Logger::LogError(error_message);
        promise->set_exception(
            std::make_exception_ptr(
                std::runtime_error(error_message)));
    }

    Logger::LogTrace("SocketWrapper::ReadFromSocketAsync returning an std::future object.");
    Logger::LogDebug("Exiting SocketWrapper::ReadFromSocketAsync");

    return future;
}

std::future<void> SocketWrapper::StartTlsAsync(boost::asio::ssl::context& context)
{
    Logger::LogDebug("Entering SocketWrapper::StartTlsAsync");
    Logger::LogTrace("SocketWrapper::StartTlsAsync parameter: ssl_context reference");

    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();

    // Attempt to get TcpSocket from m_socket
    auto tcp_socket = std::get_if<TcpSocketPtr>(&m_socket);
    if (!tcp_socket || !*tcp_socket) {
        Logger::LogError("SocketWrapper::StartTlsAsync: TcpSocket is null.");
        promise->set_exception(std::make_exception_ptr(std::runtime_error("TcpSocket is null")));
        return future;
    }

    Logger::LogProd("SocketWrapper::StartTlsAsync: TcpSocket retrieved");

    // Create SslSocket from TcpSocket
    // Use the raw pointer from the shared_ptr for the SslSocket construction
    auto ssl_socket(std::make_shared<SslSocket>(std::move(**tcp_socket), context));
    Logger::LogProd("SocketWrapper::StartTlsAsync: SslSocket created with context");

    // Perform async handshake
    ssl_socket->async_handshake(boost::asio::ssl::stream_base::server,
        [this, ssl_socket, promise](const boost::system::error_code& error) {
            if (!error) {
                Logger::LogProd("STARTTLS handshake successful");
                set_socket(ssl_socket);
                promise->set_value();
            } else {
                Logger::LogError("STARTTLS handshake error: " + error.message());
                promise->set_exception(std::make_exception_ptr(std::runtime_error(error.message())));
            }
        });

    Logger::LogTrace("SocketWrapper::StartTlsAsync returning an std::future object");
    Logger::LogDebug("Exiting SocketWrapper::StartTlsAsync");

    return future;
}

void SocketWrapper::set_timeout_timer(std::shared_ptr<boost::asio::steady_timer> timeout_timer) {
    Logger::LogDebug("Entering SocketWrapper::SetTimeoutTimer");
    Logger::LogTrace("SocketWrapper::SetTimeoutTimer params: shared_ptr to steady_timer");

    m_timeout_timer = std::move(timeout_timer);

    Logger::LogProd("Timeout timer successfully set.");
    Logger::LogDebug("Exiting SocketWrapper::SetTimeoutTimer");
}

void SocketWrapper::StartTimeoutTimer(std::chrono::seconds timeout_duration)
{
    Logger::LogDebug("Entering SocketWrapper::StartTimeoutTimer");
    Logger::LogTrace("SocketWrapper::StartTimeoutTimer params: " + std::to_string(timeout_duration.count()) + " seconds");

    if (m_timeout_timer)
    {
        m_timeout_timer->expires_after(timeout_duration);

        Logger::LogProd("Timeout timer started for " + std::to_string(timeout_duration.count()) + " seconds.");

        m_timeout_timer->async_wait([this](const boost::system::error_code& error)
        {
            if (!error)
            {
                Logger::LogWarning("Client timed out. Closing connection.");
                this->Close();
            }
            else
            {
                Logger::LogWarning("Timeout timer handler was cancelled or an error occurred: " + error.message());
            }
        });
    }
    else
    {
        Logger::LogError("Failed to start timeout timer: m_timeout_timer is null.");
    }

    Logger::LogDebug("Exiting SocketWrapper::StartTimeoutTimer");
}

void SocketWrapper::CancelTimeoutTimer()
{
    Logger::LogDebug("Entering SocketWrapper::CancelTimeoutTimer");

    if (m_timeout_timer)
    {
        boost::system::error_code ec;
        m_timeout_timer->cancel(ec);

        if (ec)
        {
            Logger::LogError("Failed to cancel timeout timer: " + ec.message());
        }
        else
        {
            Logger::LogProd("Timeout timer successfully cancelled.");
        }
    }
    else
    {
        Logger::LogError("Failed to cancel timeout timer: m_timeout_timer is null.");
    }

    Logger::LogDebug("Exiting SocketWrapper::CancelTimeoutTimer");
}

void SocketWrapper::Close()
{
    Logger::LogDebug("Entering SocketWrapper::Close");

    if (auto ssl_socket = std::get_if<SslSocketPtr>(&m_socket))
    {
        TerminateSslConnection(**ssl_socket);
    }
    else if (auto tcp_socket = std::get_if<TcpSocketPtr>(&m_socket))
    {
        TerminateTcpConnection(**tcp_socket);
    }
    Logger::LogDebug("Exiting SocketWrapper::Close");
}

    bool SocketWrapper::IsOpen() const
{
    Logger::LogDebug("Entering SocketWrapper::IsOpen");

    bool is_open = false;

    if (auto ssl_socket = std::get_if<SslSocketPtr>(&m_socket))
    {
        is_open = *ssl_socket && (*ssl_socket)->lowest_layer().is_open();
        Logger::LogProd("SocketWrapper::IsOpen: SSL socket status - " +
                        std::string(is_open ? "Open" : "Closed"));
    }
    else if (auto tcp_socket = std::get_if<TcpSocketPtr>(&m_socket))
    {
        is_open = *tcp_socket && (*tcp_socket)->is_open();
        Logger::LogProd("SocketWrapper::IsOpen: TCP socket status - " +
                        std::string(is_open ? "Open" : "Closed"));
    }

    Logger::LogDebug("Exiting SocketWrapper::IsOpen");
    Logger::LogTrace("SocketWrapper::IsOpen returning: " +
                     std::string(is_open ? "Open" : "Closed"));

    return is_open;
}


void SocketWrapper::TerminateSslConnection(SslSocket& ssl_socket)
{
    Logger::LogDebug("Entering SocketWrapper::TerminateSslConnection");

    boost::system::error_code ec;

    if (ssl_socket.lowest_layer().is_open())
    {
        Logger::LogProd("SocketWrapper::TerminateSslConnection: Shutting down SSL socket");

        ssl_socket.lowest_layer().shutdown(TcpSocket::shutdown_both, ec);
        if (ec && ec != boost::asio::error::not_connected)
        {
            Logger::LogError("SocketWrapper::TerminateSslConnection: Error shutting down SSL socket: " +
                ec.message());
        }
        else
        {
            Logger::LogProd("SocketWrapper::TerminateSslConnection: SSL socket shutdown successful");
        }

        ssl_socket.lowest_layer().cancel(ec);
        if (ec && ec != boost::asio::error::operation_aborted)
        {
            Logger::LogError("SocketWrapper::TerminateSslConnection: Error canceling SSL socket: " +
                ec.message());
        }
        else
        {
            Logger::LogProd("SocketWrapper::TerminateSslConnection: SSL socket cancel successful");
        }

        ssl_socket.lowest_layer().close(ec);
        if (ec && ec != boost::asio::error::not_connected)
        {
            Logger::LogError("SocketWrapper::TerminateSslConnection: Error closing SSL socket: " +
                ec.message());
        }
        else
        {
            Logger::LogProd("SocketWrapper::TerminateSslConnection: SSL socket closed successfully");
        }
    }

    Logger::LogDebug("Exiting SocketWrapper::TerminateSslConnection");
}

void SocketWrapper::TerminateTcpConnection(TcpSocket& tcp_socket)
{
    Logger::LogDebug("Entering SocketWrapper::TerminateTcpConnection");

    boost::system::error_code ec;

    if (tcp_socket.is_open())
    {
        Logger::LogProd("SocketWrapper::TerminateTcpConnection: Shutting down TCP socket");

        tcp_socket.shutdown(TcpSocket::shutdown_both, ec);
        if (ec && ec != boost::asio::error::not_connected)
        {
            Logger::LogError("SocketWrapper::TerminateTcpConnection: Error shutting down TCP socket: " +
                ec.message());
        }
        else
        {
            Logger::LogProd("SocketWrapper::TerminateTcpConnection: TCP socket shutdown successful");
        }

        tcp_socket.cancel(ec);
        if (ec && ec != boost::asio::error::operation_aborted)
        {
            Logger::LogError("SocketWrapper::TerminateTcpConnection: Error canceling TCP socket: " +
                ec.message());
        }
        else
        {
            Logger::LogProd("SocketWrapper::TerminateTcpConnection: TCP socket cancel successful");
        }

        tcp_socket.close(ec);
        if (ec && ec != boost::asio::error::not_connected)
        {
            Logger::LogError("SocketWrapper::TerminateTcpConnection: Error closing TCP socket: " + ec.message());
        }
        else
        {
            Logger::LogProd("SocketWrapper::TerminateTcpConnection: TCP socket closed successfully");
        }
    }

    Logger::LogDebug("Exiting SocketWrapper::TerminateTcpConnection");
}
}