#pragma once

#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <array>
#include <ctime>
#include <fstream>
#include <iostream>

#include <boost/asio.hpp>
#include <sodium.h>

#include "CommandHandler.h"
#include "MailDB/PgMailDB.h"
#include "MailMessageBuilder.h"
#include "SocketWrapper.h"
#include "ThreadPool.h"
#include "../../ErrorHandler/include/ErrorHandler.h"
#include "../../SignalHandler/include/SignalHandler.h"

using boost::asio::ip::tcp;
using namespace ISXSC;
using namespace ISXCommandHandler;
using namespace ISXErrorHandler;
using namespace ISXSignalHandler;
using namespace ISXSocketWrapper;
using namespace ISXThreadPool;

namespace ISXSS
{
/**
 * @class SmtpServer
 * @brief A class that represents an SMTP server.
 *
 * This class handles the initialization, configuration, and operation
 * of the SMTP server. It manages incoming client connections, processes
 * email messages, and handles various SMTP commands.
 */
class SmtpServer
{
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
    std::string server_name;
    std::string server_display_name;
    unsigned int port;

    size_t max_threads;

    boost::asio::steady_timer timeout_timer_;
    std::chrono::seconds timeout_seconds_;

private:
    void Accept();
private:
    MailMessageBuilder mail_builder_;
    ThreadPool<> thread_pool_;

    boost::asio::io_context& io_context_;
    boost::asio::ssl::context& ssl_context_;
    CommandHandler command_handler_;
    std::unique_ptr<tcp::acceptor> acceptor_;
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
private:
 MailMessageBuilder m_mail_builder; ///< The mail message builder for constructing email messages.
 ThreadPool<>& m_thread_pool; ///< The thread pool for managing concurrent tasks.

 boost::asio::io_context& m_io_context; ///< The Boost Asio I/O context for asynchronous operations.
 boost::asio::ssl::context& m_ssl_context; ///< The Boost Asio SSL context for secure connections.
 CommandHandler m_command_handler; ///< The command handler for processing SMTP commands.
 std::unique_ptr<tcp::acceptor> m_acceptor; ///< The TCP acceptor for accepting incoming client connections.
};
}  // namespace ISXSC

#endif  // SERVER_H
