#pragma once

#ifndef SERVERINITIALIZER_H
#define SERVERINITIALIZER_H

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <memory>
#include <string>
#include <chrono>

#include "Logger.h"
#include "ServerConfig.h"
#include "SignalHandler.h"
#include "ThreadPool.h"
#include "MailDB/PgMailDB.h"
#include "MailDB/ConnectionPool.h"
#include "MailDB/PgManager.h"

using boost::asio::ip::tcp;
using namespace ISXSignalHandler;

namespace ISXSS
{
/**
 * @class ServerInitializer
 * @brief Initializes server components using information from the configuration file.
 *
 * The `ServerInitializer` class is responsible for setting up the server's core components, including:
 * - Logging
 * - Network acceptor
 * - Thread pool
 * - Connection timeout settings
 *
 * This class provides getter methods to access various server configuration details such as:
 * - Boost.Asio I/O context
 * - Boost.Asio SSL context
 * - Network acceptor for handling incoming connections
 * - Server-specific settings like name, display name, port number, thread count, timeout duration, and log level.
 */
class ServerInitializer
{
public:
    /**
     * @brief Constructs a `ServerInitializer` instance and sets up all configuration settings.
     *
     * @param io_context The Boost.Asio I/O context used for asynchronous operations.
     * @param ssl_context The Boost.Asio SSL context used for secure communication.
     */
    ServerInitializer(boost::asio::io_context& io_context, boost::asio::ssl::context& ssl_context);

    /**
     * @brief Retrieves the server name.
     *
     * @return The server name as a string.
     */
    [[nodiscard]] std::string get_server_name() const;

    /**
     * @brief Retrieves the server display name.
     *
     * @return The server display name as a string.
     */
    [[nodiscard]] std::string get_server_display_name() const;

    /**
     * @brief Retrieves the port number on which the server is listening.
     *
     * @return The port number.
     */
    [[nodiscard]] unsigned get_port() const;

    /**
     * @brief Retrieves the maximum number of threads configured for the server.
     *
     * @return The maximum number of threads.
     */
    [[nodiscard]] size_t get_max_threads() const;

    /**
     * @brief Retrieves the timeout duration for communication.
     *
     * @return The timeout duration in seconds.
     */
    [[nodiscard]] auto get_timeout_seconds() const -> std::chrono::seconds;

    /**
     * @brief Retrieves the log level for the logging system.
     *
     * @return The log level.
     */
    [[nodiscard]] uint8_t get_log_level() const;

    /**
     * @brief Retrieves the network acceptor used for handling incoming connections.
     *
     * @return A reference to the network acceptor.
     */
    [[nodiscard]] tcp::acceptor& get_acceptor() const;

    /**
     * @brief Retrieves the thread pool used for managing concurrent tasks.
     *
     * @return A reference to the thread pool.
     */
    [[nodiscard]] auto get_thread_pool() const -> ISXThreadPool::ThreadPool<>&;

    /**
     * @brief Retrieves the Boost.Asio SSL context.
     *
     * @return The Boost.Asio SSL context.
     */
    [[nodiscard]] boost::asio::ssl::context& get_ssl_context() const;

    /**
     * @brief Retrieves the Boost.Asio I/O context.
     *
     * @return The Boost.Asio I/O context.
     */
    [[nodiscard]] boost::asio::io_context& get_io_context() const;

    /**
     * @brief Retrieves the coonection pool of connections to database.
     *
     * @return The ConnectionPool.
     */
    [[nodiscard]] ISXMailDB::ConnectionPool<pqxx::connection>& get_connection_pool() const;

    /**
     * @brief Retrieves the manager of database.
     *
     * @return The PgManager.
     */
    [[nodiscard]] ISXMailDB::PgManager& get_database_manager() const;

private:
    /**
     * @brief Initializes the logging system.
     *
     * Configures the logging system based on the settings provided in the configuration file.
     * This includes setting up the log level and configuring the logging output.
     */
    void InitializeLogging();

    /**
     * @brief Initializes the thread pool.
     *
     * Sets up the thread pool for managing concurrent tasks, based on the configuration settings.
     */
    void InitializeThreadPool();

    /**
     * @brief Initializes the connection pool to database.
     *
     */
    void InitializeDatabaseManager();

    /**
     * @brief Initializes the network acceptor.
     *
     * Configures and opens the network acceptor to listen for incoming connections on the specified port.
     * Resolves the server address and port from the configuration and binds the acceptor to this endpoint.
     */
    void InitializeAcceptor();

    /**
     * @brief Initializes the server timeout settings.
     *
     * Configures the timeout settings for communication based on the values specified in the configuration file.
     * This includes setting the timeout duration for socket operations.
     */
    void InitializeTimeout();

private:
    boost::asio::io_context& m_io_context; ///< The Boost.Asio I/O context used for asynchronous operations.
    boost::asio::ssl::context& m_ssl_context; ///< The Boost.Asio SSL context used for secure communication.
    Config m_config; ///< Configuration object containing server settings.
    std::unique_ptr<tcp::acceptor> m_acceptor; ///< Unique pointer to the network acceptor.
    std::unique_ptr<ISXThreadPool::ThreadPool<>> m_thread_pool; ///< Unique pointer to the thread pool.

    std::string m_server_name; ///< The server name.
    std::string m_server_display_name; ///< The server display name.
    unsigned m_port; ///< The port number on which the server is listening.
    std::string m_server_ip; ///< The server IP address.

    size_t m_max_threads; ///< The maximum number of threads configured for the server.
    std::chrono::seconds m_timeout_seconds; ///< The timeout duration for communication in seconds.
    uint8_t m_log_level; ///< The log level for the logging system.


    std::unique_ptr<ISXMailDB::PgManager> m_database_manager; ///< Unique pointer to the database manager.

    std::unique_ptr<ISXMailDB::ConnectionPool<pqxx::connection>> m_connection_pool; ///< Unique pointer to the connection pool.
    const uint16_t POOL_INITIAL_SIZE = 10;  ///< Initial size of the connection pool
    const std::string CONNECTION_STRING =
        "postgresql://postgres.qotrdwfvknwbfrompcji:"
        "yUf73LWenSqd9Lt4@aws-0-eu-central-1.pooler."
        "supabase.com:6543/postgres?sslmode=require"; ;  ///< Data base connection string.
};

}  // namespace ISXSS

#endif  // SERVERINITIALIZER_H
