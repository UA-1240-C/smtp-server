#pragma once

#ifndef SERVER_H
#define SERVER_H

#include <sodium.h>

#include <boost/asio.hpp>
#include <ctime>

#include "ServerInitializer.h"

using boost::asio::ip::tcp;
using namespace ISXSignalHandler;
using namespace ISXThreadPool;
using namespace ISXSignalHandler;

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

private:
    ServerInitializer m_initializer;  ///< Responsible for initializing server parameters and configurations.
};
}  // namespace ISXSS

#endif  // SERVER_H
