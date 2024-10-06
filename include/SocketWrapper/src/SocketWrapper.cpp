#include "SocketWrapper.h"

#include <utility>

namespace ISXSocketWrapper
{
SocketWrapper::SocketWrapper(const TcpSocketPtr& tcp_socket)
    : m_socket_wrapper(std::make_shared<TcpSocketManager>(tcp_socket))
{
    Logger::LogDebug("Entering SocketWrapper TCP constructor");
    Logger::LogTrace("Constructor params: TcpSocket" );
    set_timeout_timer(std::make_shared<boost::asio::steady_timer>(tcp_socket->get_executor()));
    Logger::LogDebug("Exiting SocketWrapper TCP constructor");
}

SocketWrapper::SocketWrapper(const TlsSocketPtr& ssl_socket)
    : m_socket_wrapper(std::make_shared<TlsSocketManager>(ssl_socket))
{
    Logger::LogDebug("Entering SocketWrapper SSL constructor");
    Logger::LogTrace("Constructor params: SslSocket" );
    set_timeout_timer(std::make_shared<boost::asio::steady_timer>(ssl_socket->get_executor()));
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

std::future<void> SocketWrapper::PerformTlsHandshake(boost::asio::ssl::stream_base::handshake_type handshake_type, boost::asio::ssl::context& context)
{
    Logger::LogDebug("Entering SocketWrapper::PerformTlsHandshake");
        auto promise = std::make_shared<std::promise<void>>();
        auto future = promise->get_future();

        auto* tcp_socket_manager = std::get_if<std::shared_ptr<TcpSocketManager>>(&m_socket_wrapper);
        auto tcp_socket = &(*tcp_socket_manager)->get_socket();
        
        auto ssl_socket = std::make_shared<TlsSocket>(std::move(*tcp_socket), context);


        ssl_socket->async_handshake(handshake_type,
        [promise, this, ssl_socket](const boost::system::error_code& error) {
            if (error) {
                promise->set_exception(std::make_exception_ptr(std::runtime_error(error.message())));
            } else {
                set_socket(ssl_socket);
                promise->set_value();
            }
        });
    
    Logger::LogDebug("Exiting SocketWrapper::PerformTlsHandshake");
    return future;
}

std::future<void> SocketWrapper::ResolveAndConnectAsync(boost::asio::io_context& io_context,
        const std::string& hostname, const std::string& service)
{
    Logger::LogDebug("Entering SocketWrapper::ResolveAndConnectAsync");
    Logger::LogTrace("SocketWrapper::ResolveAndConnectAsync with params: boost::asio::io::context&, const string&, const string&");
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
    Logger::LogTrace("SocketWrapper::ResolveAndConnectAsync return value: std::future<void>");
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

void SocketWrapper::ResetTimeoutTimer(std::chrono::seconds timeout_duration)
{
    Logger::LogDebug("Entering SocketWrapper::ResetTimeoutTimer");
    Logger::LogTrace("SocketWrapper::ResetTimeoutTimer params: " + std::to_string(timeout_duration.count()) + " seconds");

    CancelTimeoutTimer();
    StartTimeoutTimer(timeout_duration);

    Logger::LogDebug("Exiting SocketWrapper::ResetTimeoutTimer");
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

[[nodiscard]] TcpSocketManager* SocketWrapper::get_tcp_socket_manager() {
    if (auto* tcp_manager = std::get_if<std::shared_ptr<TcpSocketManager>>(&m_socket_wrapper)) {
        return tcp_manager->get();
    }
    return nullptr;
}


[[nodiscard]] const TcpSocketManager* SocketWrapper::get_tcp_socket_manager() const {
    if (const auto* tcp_manager = std::get_if<std::shared_ptr<TcpSocketManager>>(&m_socket_wrapper)) {
        return tcp_manager->get();
    }
    return nullptr;
}

[[nodiscard]] TlsSocketManager* SocketWrapper::get_tls_socket_manager() {
    if (auto* tls_manager = std::get_if<std::shared_ptr<TlsSocketManager>>(&m_socket_wrapper)) {
        return tls_manager->get();
    }
    return nullptr;
}

[[nodiscard]] const TlsSocketManager* SocketWrapper::get_tls_socket_manager() const {
    if (const auto* tls_manager = std::get_if<std::shared_ptr<TlsSocketManager>>(&m_socket_wrapper)) {
        return tls_manager->get();
    }
    return nullptr;
}
}

