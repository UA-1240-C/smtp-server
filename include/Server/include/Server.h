#pragma once

#ifndef SERVER_H
#define SERVER_H

#include <sodium.h>

#include <array>
#include <boost/asio.hpp>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

#include "CommandHandler.h"
#include "MailDB/PgMailDB.h"
#include "MailMessageBuilder.h"
#include "SignalHandler.h"
#include "SocketWrapper.h"
#include "ThreadPool.h"
#include "ServerConfig.h"
#include "Logger.h"

using boost::asio::ip::tcp;
using namespace ISXMM;
using namespace ISXCommandHandler;
using namespace ISXSignalHandler;
using namespace ISXSocketWrapper;
using namespace ISXThreadPool;
using namespace ISXSignalHandler;
using namespace ISXBase64;

namespace ISXSS {
/**
 * @class SmtpServer
 * @brief A class that represents an SMTP server.
 *
 * This class handles the initialization, configuration, and operation
 * of the SMTP server. It manages incoming client connections, processes
 * email messages, and handles various SMTP commands.
 */
class SmtpServer {
public:

  /**
   * @brief Constructs an SmtpServer object.
   * @param io_context The Boost Asio I/O context for handling asynchronous operations.
   * @param ssl_context The Boost Asio SSL context for secure connections.
   */
    SmtpServer(boost::asio::io_context& io_context, boost::asio::ssl::context& ssl_context);
  
  /**
   * @brief Starts the SMTP server.
   *
   * This method begins accepting incoming client connections and processing
   * their requests. It should be called to initiate server operations.
   */
    void Start();
private:
    void InitServer(const Config::Server& server_config);
    void InitTimeout(const Config::CommunicationSettings& comm_settings);
    ThreadPool<> InitThreadPool();
    void InitLogging(const Config::Logging& logging_config);

private:
	std::string m_server_name;
	std::string m_server_display_name;
	unsigned int m_port;

	size_t m_max_threads;

	boost::asio::steady_timer m_timeout_timer;
	std::chrono::seconds m_timeout_seconds;

  // std::filesystem::path m_log_filename;
  uint8_t m_log_level;
  // bool m_flush;
private:
	/**
	 * @brief Accepts incoming client connections.
	 *
	 * This method sets up an asynchronous operation to accept new client
	 * connections and handle them appropriately.
	 */
	void Accept();

	/**
	 * @brief Handles communication with a connected client.
	 * @param socket_wrapper The wrapper for the client's socket.
	 *
	 * This method processes requests from the client and manages the
	 * communication with the client.
	 */
	void HandleClient(SocketWrapper socket_wrapper);

	/**
	 * @brief Resets the timeout timer for a given socket.
	 *
	 * This method cancels any previously set timeout timer, sets a new timeout duration,
	 * and starts an asynchronous wait operation. If the timer expires before the operation
	 * is completed, it closes the associated socket.
	 *
	 * @param socket_wrapper The `SocketWrapper` instance associated with the socket
	 *        that will be monitored for timeout.
	 *
	 * @details
	 * The timeout duration is specified by the member variable `m_timeout_seconds`.
	 * The method utilizes Boost Asio's asynchronous timer functionality to handle
	 * timeout events. If a timeout occurs, the associated socket is closed.
	 * Any exceptions thrown during the timeout handling are caught and logged.
	 *
	 * @throws std::exception If an exception is thrown while handling the timeout.
	 */
	void ResetTimeoutTimer(SocketWrapper& socket_wrapper);

private:
	MailMessageBuilder m_mail_builder;	///< The mail message builder for constructing email messages.
	ThreadPool<> m_thread_pool;			///< The thread pool for managing concurrent tasks.

	boost::asio::io_context& m_io_context;		///< The Boost Asio I/O context for asynchronous operations.
	boost::asio::ssl::context& m_ssl_context;	///< The Boost Asio SSL context for secure connections.
	std::unique_ptr<tcp::acceptor> m_acceptor;	///< The TCP acceptor for accepting incoming client connections.
};
}  // namespace ISXSS

#endif	// SERVER_H
