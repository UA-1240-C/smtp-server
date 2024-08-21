#include "Server.h"

namespace ISXSS
{
SmtpServer::SmtpServer(boost::asio::io_context& io_context, boost::asio::ssl::context& ssl_context)
    : m_timeout_timer(io_context),
      m_thread_pool(InitThreadPool()),
      m_io_context(io_context),
      m_ssl_context(ssl_context)
{
    const Config config("../config.txt");
    InitLogging(config.get_logging());

    Logger::LogDebug("Entering SmtpServer constructor");
    Logger::LogTrace("Constructor params: io_context, ssl_context");

    SignalHandler::SetupSignalHandlers();
    InitServer(config.get_server());
    InitTimeout(config.get_communication_settings());

    Logger::LogProd("SmtpServer initialized and listening on port " + std::to_string(m_port));
    Logger::LogDebug("Exiting SmtpServer constructor");
}

void SmtpServer::InitServer(const Config::Server& server_config) {
    Logger::LogDebug("Entering InitServer");
    Logger::LogTrace("InitServer params: {server_name: " + server_config.server_name +
                     ", server_display_name: " + server_config.server_display_name +
                     ", listener_port: " + std::to_string(server_config.listener_port) + "}");

    m_port = server_config.listener_port;
    m_server_display_name = server_config.server_display_name;
    m_server_name = server_config.server_name;
    m_acceptor = std::make_unique<tcp::acceptor>(m_io_context, tcp::endpoint(tcp::v4(), m_port));

    Logger::LogTrace("Initialized server with server_name: " + m_server_name +
                     ", server_display_name: " + m_server_display_name +
                     ", listener_port: " + std::to_string(m_port));
    Logger::LogDebug("Exiting InitServer");
}

void SmtpServer::InitTimeout(const Config::CommunicationSettings& comm_settings) {
    Logger::LogDebug("Entering InitTimeout");
    Logger::LogTrace("InitTimeout params: {socket_timeout: " +
            std::to_string(comm_settings.socket_timeout) + "}");

    m_timeout_seconds = std::chrono::seconds(comm_settings.socket_timeout);

    Logger::LogTrace("Timeout initialized to " +
            std::to_string(m_timeout_seconds.count()) + " seconds");
    Logger::LogDebug("Exiting InitTimeout");
}

ThreadPool<> SmtpServer::InitThreadPool() {
    Logger::LogDebug("Entering InitThreadPool");

    const Config config("../config.txt");
    auto [max_working_threads] = config.get_thread_pool();

    Logger::LogTrace("InitThreadPool params: {max_working_threads: " +
            std::to_string(max_working_threads) + "}");

    const size_t max_threads = max_working_threads > std::thread::hardware_concurrency()
        ? std::thread::hardware_concurrency()
        : max_working_threads;

    Logger::LogTrace("Thread pool initialized with " +
            std::to_string(max_threads) + " threads");
    Logger::LogDebug("Exiting InitThreadPool");

    return ThreadPool<>(max_threads);
}

void SmtpServer::InitLogging(const Config::Logging& logging_config) {
    Logger::LogDebug("Entering InitLogging");
    Logger::LogTrace("InitLogging params: {log_level: " +
            std::to_string(logging_config.log_level) + "}");

    Logger::Setup(logging_config.log_level);

    Logger::LogTrace("Logging initialized with log_level: " +
            std::to_string(logging_config.log_level));
    Logger::LogDebug("Exiting InitLogging");
}

void SmtpServer::Start()
{
    Logger::LogDebug("Entering Start");

    Accept();

    Logger::LogDebug("Exiting Start");
}

void SmtpServer::Accept() {
    Logger::LogDebug("Entering SmtpServer::Accept");
    auto new_socket = std::make_shared<TcpSocket>(m_io_context);

    Logger::LogProd("Ready to accept new connections.");


    m_acceptor->async_accept(*new_socket, [this, new_socket](const boost::system::error_code& error) {
        if (!error) {
            Logger::LogProd("Accepted new connection.");
            m_thread_pool.EnqueueDetach([this, new_socket]() { HandleClient(SocketWrapper(new_socket)); });
        } else {
            Logger::LogError("Boost error in SmtpServer::Accept" +
                error.what());
        }
        Accept();
    });
    Logger::LogDebug("Exiting Accept");
}

void SmtpServer::ResetTimeoutTimer(SocketWrapper& socket_wrapper) {
	Logger::LogDebug("Entering SmtpServer::ResetTimeoutTimer");
	Logger::LogTrace("SmtpServer::ResetTimeoutTimer params: SocketWrapper reference");

	auto timeout_timer = std::make_shared<boost::asio::steady_timer>(m_io_context);
	socket_wrapper.SetTimeoutTimer(timeout_timer);
	socket_wrapper.StartTimeoutTimer(m_timeout_seconds);

	Logger::LogDebug("Exiting SmtpServer::ResetTimeoutTimer");
}

void SmtpServer::HandleClient(SocketWrapper socket_wrapper) {
    Logger::LogDebug("Entering SmtpServer::HandleClient");
    socket_wrapper.SendResponseAsync("220 Client was successfully connected!\r\n").get();
    Logger::LogTrace("SmtpServer::HandleClient parameters: SocketWrapper");

    try {
        CommandHandler command_handler(m_ssl_context);
        MailMessageBuilder mail_builder;
        std::string current_line;

        while (true) {
            constexpr size_t max_length = 1024;
            if (!socket_wrapper.IsOpen()) break;

            Logger::LogProd("Reading data from socket.");

            auto future_data = socket_wrapper.ReadFromSocketAsync(max_length);

            try {
                std::string buffer = future_data.get();

                Logger::LogProd("Received data: " + buffer);

                current_line.append(buffer);
                std::size_t pos;
                while ((pos = current_line.find("\r\n")) != std::string::npos) {
                    std::string line = current_line.substr(0, pos);
                    current_line.erase(0, pos + 2);

                    Logger::LogProd("Processing line: " + line);

                    command_handler.ProcessLine(line, socket_wrapper);
                }

				socket_wrapper.CancelTimeoutTimer();
				ResetTimeoutTimer(socket_wrapper);
			} catch (const boost::system::system_error& e) {
                if (e.code() == boost::asio::error::operation_aborted) {
                    Logger::LogWarning("Client disconnected.");
                    break;
                }
                Logger::LogError("Read from socket" + std::string(e.what()));
                throw;
            } catch (const std::exception& e) {
                Logger::LogError("Read from socket" + std::string(e.what()));
                throw;
            }
        }
    } catch (std::exception& e) {
        Logger::LogError("SmtpServer::HandleClient: Exception occurred - " + std::string(e.what()));
    }

    Logger::LogDebug("Exiting SmtpServer::HandleClient");
}
}