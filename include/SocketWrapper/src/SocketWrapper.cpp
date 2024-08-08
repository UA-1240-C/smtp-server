#include "SocketWrapper.h"

namespace ISXSocketWrapper
{
SocketWrapper::SocketWrapper(std::shared_ptr<TcpSocket> tcp_socket)
    : m_socket(tcp_socket), m_is_tls(false) {}

SocketWrapper::SocketWrapper(std::shared_ptr<SslSocket> ssl_socket)
    : m_socket(ssl_socket), m_is_tls(true) {}

[[nodiscard]] bool SocketWrapper::IsTls() const
{
    return m_is_tls;
}

std::future<void> SocketWrapper::SendResponseAsync(const std::string& message)
{
    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();

    if (auto tcp_socket = get_socket<TcpSocket>())
    {
        async_write(*tcp_socket, boost::asio::buffer(message),
            [promise](const boost::system::error_code& error, std::size_t) {
                if (error)
                {
                    promise->set_exception(std::make_exception_ptr(std::runtime_error(error.message())));
                }
                else
                {
                    promise->set_value();
                }
            });
    }
    else if (auto ssl_socket = get_socket<SslSocket>())
    {
        async_write(*ssl_socket, boost::asio::buffer(message),
            [promise](const boost::system::error_code& error, std::size_t) {
                if (error) {
                    promise->set_exception(std::make_exception_ptr(std::runtime_error(error.message())));
                } else {
                    promise->set_value();
                }
            });
    }
    else
    {
        promise->set_exception(
            std::make_exception_ptr(
                std::runtime_error("No valid socket available for sending data.")));
    }

    return future;
}

std::future<std::string> SocketWrapper::ReadFromSocketAsync(size_t max_length)
{
    auto promise = std::make_shared<std::promise<std::string>>();
    auto future = promise->get_future();
    auto buffer = std::make_shared<std::string>(max_length, '\0');

    if (auto tcp_socket = get_socket<TcpSocket>())
    {
        tcp_socket->async_read_some(boost::asio::buffer(*buffer),
            [promise, buffer](const boost::system::error_code& error, std::size_t length)
            {
                if (error)
                {
                    promise->set_exception(std::make_exception_ptr(std::runtime_error(error.message())));
                }
                else
                {
                    buffer->resize(length);
                    promise->set_value(*buffer);
                }
            });
    }
    else if (auto ssl_socket = get_socket<SslSocket>())
    {
        ssl_socket->async_read_some(boost::asio::buffer(*buffer),
            [promise, buffer](const boost::system::error_code& error, std::size_t length) {
                if (error)
                {
                    promise->set_exception(
                        std::make_exception_ptr(std::runtime_error(error.message())));
                } else
                {
                    buffer->resize(length);
                    promise->set_value(*buffer);
                }
            });
    }
    else
    {
        promise->set_exception(
            std::make_exception_ptr(
                std::runtime_error("No valid socket available for reading data.")));
    }

    return future;
}

std::future<void> SocketWrapper::StartTlsAsync(boost::asio::ssl::context& context)
{
    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();

    auto tcp_socket = get_socket<TcpSocket>();
    auto ssl_socket = std::make_shared<SslSocket>(std::move(*tcp_socket), context);

    ssl_socket->set_verify_mode(boost::asio::ssl::verify_none);

    ssl_socket->async_handshake(boost::asio::ssl::stream_base::server,
        [this, ssl_socket, promise](const boost::system::error_code& error) {
            if (!error)
            {
                // Successfully established TLS connection
                m_socket = ssl_socket;
                std::cout << "STARTTLS handshake successful" << std::endl;
                promise->set_value();
            }
            else
            {
                std::cerr << "STARTTLS handshake error: " << error.message() << std::endl;
                promise->set_exception(
                    std::make_exception_ptr(std::runtime_error(error.message())));
            }
        });

    return future;
}
}