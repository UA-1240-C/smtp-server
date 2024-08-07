#include "SocketWrapper.h"

SocketWrapper::SocketWrapper(std::shared_ptr<TcpSocket> tcp_socket)
    : socket_(tcp_socket), is_tls_(false) {}

SocketWrapper::SocketWrapper(std::shared_ptr<SslSocket> ssl_socket)
    : socket_(ssl_socket), is_tls_(true) {}

[[nodiscard]] bool SocketWrapper::is_tls() const {
    return std::holds_alternative<std::shared_ptr<SslSocket>>(socket_);
}
/*
void SocketWrapper::sendResponse(const std::string& message) {
    std::cout << "Sending response: " << message << std::endl;
    if (auto tcp_socket = get<TcpSocket>()) {
        boost::asio::write(*tcp_socket, boost::asio::buffer(message));
    } else if (auto ssl_socket = get<SslSocket>()) {
        boost::asio::write(*ssl_socket, boost::asio::buffer(message));
    } else {
        std::cerr << "Error: No valid socket available for sending data."
                  << std::endl;
    }
}*/

std::future<void> SocketWrapper::sendResponseAsync(const std::string& message) {
    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();

    if (auto tcp_socket = get<boost::asio::ip::tcp::socket>()) {
        async_write(*tcp_socket, boost::asio::buffer(message),
            [promise](const boost::system::error_code& error, std::size_t) {
                if (error) {
                    promise->set_exception(std::make_exception_ptr(std::runtime_error(error.message())));
                } else {
                    promise->set_value();
                }
            });
    } else if (auto ssl_socket = get<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>()) {
        async_write(*ssl_socket, boost::asio::buffer(message),
            [promise](const boost::system::error_code& error, std::size_t) {
                if (error) {
                    promise->set_exception(std::make_exception_ptr(std::runtime_error(error.message())));
                } else {
                    promise->set_value();
                }
            });
    } else {
        promise->set_exception(std::make_exception_ptr(std::runtime_error("No valid socket available for sending data.")));
    }

    return future;
}

std::future<std::string> SocketWrapper::readFromSocketAsync(size_t max_length) {
    auto promise = std::make_shared<std::promise<std::string>>();
    auto future = promise->get_future();
    auto buffer = std::make_shared<std::string>(max_length, '\0');

    if (auto tcp_socket = get<boost::asio::ip::tcp::socket>()) {
        tcp_socket->async_read_some(boost::asio::buffer(*buffer),
            [promise, buffer](const boost::system::error_code& error, std::size_t length) {
                if (error) {
                    promise->set_exception(std::make_exception_ptr(std::runtime_error(error.message())));
                } else {
                    buffer->resize(length);
                    promise->set_value(*buffer);
                }
            });
    } else if (auto ssl_socket = get<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>()) {
        ssl_socket->async_read_some(boost::asio::buffer(*buffer),
            [promise, buffer](const boost::system::error_code& error, std::size_t length) {
                if (error) {
                    promise->set_exception(std::make_exception_ptr(std::runtime_error(error.message())));
                } else {
                    buffer->resize(length);
                    promise->set_value(*buffer);
                }
            });
    } else {
        promise->set_exception(std::make_exception_ptr(std::runtime_error("No valid socket available for reading data.")));
    }

    return future;
}

std::future<void> SocketWrapper::startTlsAsync(boost::asio::ssl::context& context) {
    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();

    auto tcp_socket = get<boost::asio::ip::tcp::socket>();
    auto ssl_socket = std::make_shared<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>(std::move(*tcp_socket), context);

    ssl_socket->set_verify_mode(boost::asio::ssl::verify_none);

    ssl_socket->async_handshake(boost::asio::ssl::stream_base::server,
        [this, ssl_socket, promise](const boost::system::error_code& error) {
            if (!error) {
                // Successfully established TLS connection
                socket_ = ssl_socket;
                std::cout << "STARTTLS handshake successful" << std::endl;
                promise->set_value();
            } else {
                std::cerr << "STARTTLS handshake error: " << error.message() << std::endl;
                promise->set_exception(std::make_exception_ptr(std::runtime_error(error.message())));
            }
        });

    return future;
}