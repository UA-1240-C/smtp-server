#include "SocketWrapper.h"

#include <utility>

namespace ISXSocketWrapper
{
SocketWrapper::SocketWrapper(const TcpSocketPtr& tcp_socket)
    : m_socket_wrapper(std::make_shared<TcpSocketManager>(tcp_socket))
{
    Logger::LogDebug("Entering SocketWrapper TCP constructor");
    Logger::LogTrace("Constructor params: TcpSocket" );

    Logger::LogDebug("Exiting SocketWrapper TCP constructor");
}

SocketWrapper::SocketWrapper(const TlsSocketPtr& ssl_socket)
    : m_socket_wrapper(std::make_shared<TlsSocketManager>(ssl_socket))
{
    Logger::LogDebug("Entering SocketWrapper SSL constructor");
    Logger::LogTrace("Constructor params: SslSocket" );

    Logger::LogDebug("Exiting SocketWrapper SSL constructor");
}

[[nodiscard]] bool SocketWrapper::IsTls() const
{
    Logger::LogDebug("Entering SocketWrapper::IsTls");

    Logger::LogTrace("SocketWrapper::IsTls return value: bool");
    Logger::LogDebug("Exiting SocketWrapper::IsTls");
    return std::holds_alternative<std::shared_ptr<TlsSocketManager>>(m_socket_wrapper);
}

std::future<void> SocketWrapper::WriteAsync(const std::string& message)
{
    if (IsTls()) {
        return std::get<std::shared_ptr<TlsSocketManager>>(m_socket_wrapper)->WriteAsync(message);
    }
    return std::get<std::shared_ptr<TcpSocketManager>>(m_socket_wrapper)->WriteAsync(message);
}


std::future<std::string> SocketWrapper::ReadAsync(size_t max_length) const
{
    if (IsTls()) {
        return std::get<std::shared_ptr<TlsSocketManager>>(m_socket_wrapper)->ReadAsync(max_length);
    }
    return std::get<std::shared_ptr<TcpSocketManager>>(m_socket_wrapper)->ReadAsync(max_length);
}

void SocketWrapper::UpgradeToTls(std::shared_ptr<TcpSocket> tcp_socket)
{
    if (IsTls()) {
        throw std::runtime_error("Socket is already using TLS.");
    }

    boost::asio::ssl::context ssl_context(boost::asio::ssl::context::tlsv12_client);
    ssl_context.set_default_verify_paths();

    
    auto ssl_socket = std::make_shared<TlsSocket>(std::move(*tcp_socket), ssl_context);

    set_socket(ssl_socket);
}


std::future<void> SocketWrapper::PerformTlsHandshake(boost::asio::ssl::stream_base::handshake_type handshake_type) const
{
    Logger::LogDebug("Entering SocketWrapper::PerformTlsHandshake");
    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();
    if (IsTls())
    {
        if (handshake_type == boost::asio::ssl::stream_base::server) {
            future = std::get<std::shared_ptr<TlsSocketManager>>(m_socket_wrapper)
            ->PerformTlsHandshake(boost::asio::ssl::stream_base::handshake_type::server);
        } else {
            Logger::LogDebug("Here 1");
            future = std::get<std::shared_ptr<TlsSocketManager>>(m_socket_wrapper)
            ->PerformTlsHandshake(boost::asio::ssl::stream_base::handshake_type::client);
        }
    }
    Logger::LogDebug("Exiting SocketWrapper::PerformTlsHandshake");
    return future;
}

    std::future<void> SocketWrapper::ResolveAndConnectAsync(boost::asio::io_context& io_context,
        const std::string& hostname, const std::string& service)
{
    Logger::LogDebug("Entering SocketWrapper::ResolveAndConnectAsync");
    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();

    auto resolver = std::make_shared<boost::asio::ip::tcp::resolver>(io_context);

    // Check if the stored socket in m_socket_wrapper is of type TcpSocketManager
    if (std::holds_alternative<std::shared_ptr<TcpSocketManager>>(m_socket_wrapper)) {
        auto tcp_socket_manager = std::get<std::shared_ptr<TcpSocketManager>>(m_socket_wrapper);

        resolver->async_resolve(hostname, service,
            [promise, tcp_socket_manager, resolver](const boost::system::error_code& ec,
                                                    boost::asio::ip::tcp::resolver::results_type results) {
                if (ec) {
                    promise->set_exception(std::make_exception_ptr(
                        std::runtime_error("Failed to resolve: " + ec.message())));
                    return;
                }

                async_connect(tcp_socket_manager->get_socket(), results,
                                           [promise](const boost::system::error_code& ec, const auto&) {
                                               if (ec) {
                                                   promise->set_exception(std::make_exception_ptr(
                                                       std::runtime_error("Failed to connect: " + ec.message())));
                                               } else {
                                                   promise->set_value();
                                               }
                                           });
            });

    } else {
        promise->set_exception(std::make_exception_ptr(
            std::runtime_error("Socket is not a TcpSocketManager")));
    }
    Logger::LogDebug("Exiting SocketWrapper::ResolveAndConnectAsync");
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
                Close();
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

    if (IsTls())
    {
        std::get<std::shared_ptr<TlsSocketManager>>(m_socket_wrapper)
            ->TerminateConnection();
    }
    std::get<std::shared_ptr<TcpSocketManager>>(m_socket_wrapper)
            ->TerminateConnection();
    Logger::LogDebug("Exiting SocketWrapper::Close");
}

bool SocketWrapper::IsOpen() const
{
    Logger::LogDebug("Entering SocketWrapper::IsOpen");

    if (IsTls()) {
        return std::get<std::shared_ptr<TlsSocketManager>>(m_socket_wrapper)->IsOpen();
    }
    return std::get<std::shared_ptr<TcpSocketManager>>(m_socket_wrapper)->IsOpen();
}

}