#pragma once

#ifndef SERVER_H
#define SERVER_H

#include <sodium.h>

#include <array>
#include <boost/asio.hpp>
#include <ctime>
#include <fstream>
#include <string>

#include "CommandHandler.h"
#include "Logger.h"
#include "MailMessageBuilder.h"
#include "ServerInitializer.h"
#include "SignalHandler.h"
#include "SocketWrapper.h"
#include "StandartSmtpResponses.h"
#include "ThreadPool.h"

using boost::asio::ip::tcp;
using namespace ISXMM;
using namespace ISXCommandHandler;
using namespace ISXSignalHandler;
using namespace ISXSocketWrapper;
using namespace ISXThreadPool;
using namespace ISXSignalHandler;
using namespace ISXBase64;

constexpr std::size_t DELIMITER_OFFSET = 2;

namespace ISXSS
{

 /**
  * @class SmtpServer
  * @brief A class that represents an SMTP server.
  *
  * The `SmtpServer` class encapsulates the logic and state necessary for operating an SMTP server.
  * It is responsible for handling client connections, managing incoming email messages,
  * and processing various SMTP commands such as HELO, MAIL, RCPT, DATA, and QUIT.
  *
  * This class leverages Boost Asio for asynchronous I/O operations,
  * ensuring efficient handling of multiple simultaneous client connections.
  * Additionally, it supports secure connections through SSL/TLS by utilizing the Boost Asio SSL context.
  *
  * The server is designed to be robust and scalable, with provisions for handling timeouts,
  * logging activities, and safely managing resources using RAII principles and smart pointers.
  */
class SmtpServer
{
public:
   /**
    * @brief Constructs an SmtpServer object.
    * @param io_context The Boost Asio I/O context for handling asynchronous operations.
    * @param ssl_context The Boost Asio SSL context for secure connections.
    *
    * The constructor initializes the server with the provided I/O and SSL contexts.
    * It prepares the server to start accepting client connections and handling their requests.
    *
    * @note The I/O context (`io_context`) is required for managing all asynchronous operations,
    * while the SSL context (`ssl_context`) is used for establishing and managing secure SSL/TLS connections.
    */
    SmtpServer(boost::asio::io_context& io_context, boost::asio::ssl::context& ssl_context);

    /**
     * @brief Starts the SMTP server.
     *
     * This method begins the server's operation by initiating the asynchronous accept loop.
     * It starts listening for incoming client connections and sets up the necessary handlers
     * for processing client requests.
     *
     * @details
     * The server will continue to run, accepting and processing client connections until it is explicitly stopped.
     * Each client connection is handled by a separate asynchronous operation, ensuring non-blocking behavior.
     *
     * @note This method should be called after constructing the `SmtpServer` object to start the server.
     */
    void Start();

private:
   /**
    * @brief Accepts incoming client connections.
    *
    * This method creates a new socket for the new connection session and initiates an asynchronous
    * accept operation. When a client attempts to connect, the server accepts the connection and
    * hands it off to a handler for further processing.
    *
    * @details
    * The accept operation uses Boost Asio's asynchronous mechanisms to ensure that the server
    * remains responsive to other clients while waiting for connections. Upon accepting a connection,
    * the method immediately starts listening for the next connection.
    *
    * @note The server supports multiple concurrent connections, with each connection handled independently.
    */
    void Accept();

    /**
     * @brief Handles communication with a connected client.
     * @param socket_wrapper The wrapper for the client's socket.
     *
     * This method is responsible for managing the entire lifecycle of a client's interaction with the server.
     * It processes SMTP commands from the client, constructs appropriate responses, and ensures that the
     * communication adheres to the SMTP protocol.
     *
     * @details
     * The method reads data from the client, interprets SMTP commands, and invokes the appropriate handlers
     * (such as handling the HELO, MAIL, RCPT, DATA, and QUIT commands). It also manages the sending of responses
     * back to the client.
     *
     * In case of errors or invalid commands, the method sends standard SMTP error responses.
     * If the client connection is secure (using SSL/TLS), the method ensures that all data exchanges are encrypted.
     *
     * @note Each client connection runs in its own asynchronous context, allowing the server to handle multiple
     * clients simultaneously without blocking.
     */
    void HandleClient(SocketWrapper socket_wrapper);

    /**
     * @brief Resets the timeout timer for a given socket.
     *
     * This method manages the connection timeout for a client. If the client fails to send a command
     * within the specified timeout period, the connection is closed to prevent resource exhaustion.
     *
     * @param socket_wrapper The `SocketWrapper` instance associated with the socket that will be monitored for timeout.
     *
     * @details
     * The timeout duration is governed by `m_timeout_seconds`. The method first cancels any previously set timeout
     * to avoid unintended disconnections. It then sets a new timeout and starts an asynchronous wait on the timer.
     *
     * If the timer expires before the client completes its operation, the socket is forcefully closed to terminate the connection.
     * This helps in preventing potential DoS attacks where a client may hold onto a connection indefinitely.
     *
     * The method also logs any exceptions encountered during the timeout handling, ensuring that the server can diagnose
     * and recover from issues gracefully.
     *
     * @throws std::exception If an exception is thrown while handling the timeout.
     *
     * @note This method is crucial for maintaining the server's performance and preventing misuse of resources.
     */
    void ResetTimeoutTimer(SocketWrapper& socket_wrapper);
private:
    ServerInitializer m_initializer;  ///< Responsible for initializing server parameters and configurations.
    MailMessageBuilder m_mail_builder;  ///< The mail message builder for constructing email messages from client data.
    boost::asio::steady_timer m_timeout_timer;  ///< Timer to manage client connection timeouts.
};
}  // namespace ISXSS

#endif  // SERVER_H
