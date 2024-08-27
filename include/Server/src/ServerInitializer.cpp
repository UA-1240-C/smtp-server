#include "ServerInitializer.h"

#include <utility>

namespace ISXSS
{
ServerInitializer::ServerInitializer(boost::asio::io_context& io_context, boost::asio::ssl::context& ssl_context)
    : m_io_context(io_context), m_ssl_context(ssl_context), m_config("../config.txt")
{
    InitializeLogging();
    InitializeAcceptor();
    InitializeTimeout();
    InitializeThreadPool();
    SignalHandler::SetupSignalHandlers();
}

void ServerInitializer::InitializeAcceptor()
{
    Logger::LogDebug("Entering ServerInitializer::InitializeAcceptor.");

    const auto& [server_name, server_display_name, listener_port, ip_address] = m_config.get_server();
    m_server_name = server_name;
    m_server_display_name = server_display_name;
    m_port = listener_port;
    m_server_ip = ip_address;

    try
    {
        tcp::resolver resolver(m_io_context);  // DNS resovler to uses to resolve a server name to IP addess
        tcp::resolver::query query(m_server_ip, std::to_string(m_port));
        auto endpoints = resolver.resolve(query);

        if (endpoints.empty())
        {
            throw std::runtime_error("No endpoints resolved");
        }

        tcp::endpoint endpoint = *endpoints.begin();
        Logger::LogProd("Endpoint resolved to: " + endpoint.address().to_string() + ":" +
                        std::to_string(endpoint.port()));

        m_acceptor = std::make_unique<tcp::acceptor>(m_io_context);
        m_acceptor->open(endpoint.protocol());
        m_acceptor->set_option(tcp::acceptor::reuse_address(true));
        m_acceptor->bind(endpoint);
        m_acceptor->listen();

        Logger::LogProd("Acceptor initialized and listening on port " + std::to_string(m_port));
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception in InitializeAcceptor: " + std::string(e.what()));
        throw;
    }

    Logger::LogDebug("Exiting ServerInitializer::InitializeAcceptor.");
}

void ServerInitializer::InitializeLogging()
{
    Logger::LogDebug("Entering ServerInitializer::InitializeLogging.");

    const auto& [filename, log_level, flush] = m_config.get_logging();

    m_log_level = log_level;
    Logger::Setup(m_config.get_logging());

    Logger::LogTrace("Logging initialized with log_level: " + std::to_string(log_level));
    Logger::LogDebug("Exiting ServerInitializer::InitializeLogging.");
}

void ServerInitializer::InitializeThreadPool()
{
    Logger::LogDebug("Entering ServerInitializer::InitializeThreadPool");

    auto [max_working_threads] = m_config.get_thread_pool();

    Logger::LogTrace("InitThreadPool params: {max_working_threads: " + std::to_string(max_working_threads) + "}");

    const size_t max_threads = max_working_threads > std::thread::hardware_concurrency()
                                   ? std::thread::hardware_concurrency()
                                   : max_working_threads;

    auto thread_pool = std::make_unique<ISXThreadPool::ThreadPool<>>(max_threads);
    m_thread_pool = std::move(thread_pool);

    Logger::LogTrace("Thread pool initialized with " + std::to_string(max_threads) + " threads");
    Logger::LogDebug("Exiting ServerInitializer::InitializeThreadPool");
}

void ServerInitializer::InitializeTimeout()
{
    Logger::LogDebug("Entering ServerInitializer::InitializeTimeout");
    const auto& [blocking, socket_timeout] = m_config.get_communication_settings();

    m_timeout_seconds = std::chrono::seconds(socket_timeout);

    Logger::LogDebug("Timeout initialized to " + std::to_string(m_timeout_seconds.count()) + " seconds");
    Logger::LogDebug("Exiting ServerInitializer::InitializeTimeout");
}

auto ServerInitializer::get_thread_pool() const -> ISXThreadPool::ThreadPool<>& { return *m_thread_pool; }

boost::asio::ssl::context& ServerInitializer::get_ssl_context() const { return m_ssl_context; }

boost::asio::io_context& ServerInitializer::get_io_context() const { return m_io_context; }

std::string ServerInitializer::get_server_name() const { return m_server_name; }

std::string ServerInitializer::get_server_display_name() const { return m_server_display_name; }

unsigned ServerInitializer::get_port() const { return m_port; }

size_t ServerInitializer::get_max_threads() const { return m_max_threads; }

auto ServerInitializer::get_timeout_seconds() const -> std::chrono::seconds { return m_timeout_seconds; }

uint8_t ServerInitializer::get_log_level() const { return m_log_level; }

tcp::acceptor& ServerInitializer::get_acceptor() const { return *m_acceptor; }
}  // namespace ISXSS
