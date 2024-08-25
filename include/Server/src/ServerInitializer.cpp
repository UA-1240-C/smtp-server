#include "ServerInitializer.h"

namespace ISXSS
{
    ServerInitializer::ServerInitializer(boost::asio::io_context& io_context, boost::asio::ssl::context& ssl_context)
        : m_io_context(io_context)
        , m_ssl_context(ssl_context)
        , m_config("../config.txt")
    {
        InitializeLogging();
        InitializeThreadPool();
        InitializeAcceptor();
        InitializeTimeout();
        SignalHandler::SetupSignalHandlers();
    }

    void ServerInitializer::InitializeAcceptor()
    {
        const Config::Server& server_config = m_config.get_server();

        set_server_name(server_config.server_name);
		set_server_display_name(server_config.server_display_name);
		set_port(server_config.listener_port);

        boost::asio::ip::tcp::resolver resolver(m_io_context);
        boost::asio::ip::tcp::resolver::query query(m_server_name, std::to_string(m_port));
        boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);

        m_acceptor = std::make_unique<boost::asio::ip::tcp::acceptor>(m_io_context);
        m_acceptor->open(endpoint.protocol());
        m_acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        m_acceptor->bind(endpoint);
        m_acceptor->listen();

        Logger::LogDebug("Acceptor initialized and listening on port " + std::to_string(m_port));
    }

	void ServerInitializer::InitializeLogging()
    {
        const Config::Logging& logging_config = m_config.get_logging();

        m_log_level = logging_config.log_level;
        Logger::Setup(m_log_level);

        Logger::LogDebug("Logging initialized with log_level: " + std::to_string(logging_config.log_level));
    }

    void ServerInitializer::InitializeThreadPool()
    {
        m_thread_pool = std::make_unique<ISXThreadPool::ThreadPool<>>(set_thread_pool_impl());
        Logger::LogDebug("Thread pool initialized with " + std::to_string(max_threads) + " threads");
    }

    void ServerInitializer::InitializeTimeout()
    {
        const Config::CommunicationSettings& comm_settings = m_config.get_communication_settings();

        set_timeout_seconds(std::chrono::seconds(comm_settings.socket_timeout));

        Logger::LogDebug("Timeout initialized to " + std::to_string(m_timeout_seconds.count()) + " seconds");
    }

    std::string ServerInitializer::get_server_name() {
        return m_server_name;
    }

    std::string ServerInitializer::get_server_display_name() {
        return m_server_display_name;
    }

    unsigned ServerInitializer::get_port() {
        return m_port;
    }

    size_t ServerInitializer::get_max_threads() {
        return m_max_threads;
    }

    auto ServerInitializer::get_timeout_seconds() -> std::chrono::seconds {
        return m_timeout_seconds;
    }

    uint8_t ServerInitializer::get_log_level() {
        return m_log_level;
    }
/*
    void ServerInitializer::set_server_name(std::string server_name) {
        m_server_name = server_name;
    }

    void ServerInitializer::set_server_display_name(std::string server_display_name) {
        m_server_display_name = server_display_name;
    }

    void ServerInitializer::set_port(unsigned port) {
        m_port = port;
    }
*/
    void ServerInitializer::set_max_threads() {
        auto [max_working_threads] = m_config.get_thread_pool();

        const size_t max_threads = max_working_threads > std::thread::hardware_concurrency()
                                   ? std::thread::hardware_concurrency()
                                   : max_working_threads;
        m_max_threads = max_threads;
    }

    void ServerInitializer::set_timeout_seconds(std::chrono::seconds timeout_seconds) {
        m_timeout_seconds = timeout_seconds;
    }

    void ServerInitializer::set_log_level(uint8_t log_level) {
        m_log_level = log_level;
    }

    ISXThreadPool::ThreadPool<> ServerInitializer::get_thread_pool_impl() {
        return ISXThreadPool::ThreadPool<>(get_max_threads());
    }
}
