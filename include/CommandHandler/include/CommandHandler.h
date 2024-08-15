#pragma once

#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include <string>
#include <utility>
#include <iostream>

#include <sodium/crypto_pwhash.h>
#include <boost/asio/ssl.hpp>

#include "Base64.h"
#include "MailDB/PgMailDB.h"
#include "MailMessageBuilder.h"
#include "SocketWrapper.h"
#include "ErrorHandler.h"

using namespace ISXErrorHandler;
using namespace ISXSC;
using namespace ISXSocketWrapper;
using namespace ISXMailDB;

namespace ISXCommandHandler
{
/**
 * @class CommandHandler
 * @brief Handles SMTP commands and interactions with the mail database.
 *
 * This class processes various SMTP commands and manages interactions
 * with the mail database and mail message builder. It handles commands
 * such as MAIL FROM, RCPT TO, DATA, AUTH, and others.
 */
class CommandHandler
{
public:
    /**
     * @brief Constructs a CommandHandler object.
     * @param ssl_context Reference to the SSL context for secure connections.
     */
    CommandHandler(boost::asio::ssl::context& ssl_context);

    /**
     * @brief Destructs the CommandHandler object.
     */
    ~CommandHandler();

    /**
     * @brief Processes a line of input from the client.
     * @param line The line of input to process.
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     */
    void ProcessLine(const std::string& line,
                     SocketWrapper& socket_wrapper);
private:
    /**
     * @brief Handles the MAIL FROM command.
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     * @param line The line of input containing the MAIL FROM command.
     */
    void HandleMailFrom(SocketWrapper& socket_wrapper, const std::string& line);

    /**
     * @brief Handles the RCPT TO command.
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     * @param line The line of input containing the RCPT TO command.
     */
    void HandleRcptTo(SocketWrapper& socket_wrapper, const std::string& line);

    /**
     * @brief Handles the VRFY command.
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     * @param line The line of input containing the VRFY command.
     */
    void HandleVrfy(SocketWrapper& socket_wrapper, const std::string& line);

    /**
     * @brief Handles the EXPN command.
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     * @param line The line of input containing the EXPN command.
     */
    void HandleExpn(SocketWrapper& socket_wrapper, const std::string& line);

    /**
     * @brief Handles the NOOP command.
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     */
    void HandleNoop(SocketWrapper& socket_wrapper);

    /**
     * @brief Handles the RSET command.
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     */
    void HandleRset(SocketWrapper& socket_wrapper);

    /**
     * @brief Handles the HELP command.
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     */
    void HandleHelp(SocketWrapper& socket_wrapper);

    /**
     * @brief Handles the EHLO command.
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     */
    void HandleEhlo(SocketWrapper& socket_wrapper);

    /**
     * @brief Handles the STARTTLS command.
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     */
    void HandleStartTLS(SocketWrapper& socket_wrapper);

    /**
     * @brief Handles the DATA command.
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     */
    void HandleData(SocketWrapper& socket_wrapper);

    /**
     * @brief Reads data from the client during DATA context.
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     * @param data_message String to store the received data.
     */
    void ReadData(SocketWrapper& socket_wrapper, std::string& data_message);

    /**
     * @brief Processes the received data message.
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     * @param data_message The data message to process.
     */
    void ProcessDataMessage(SocketWrapper& socket_wrapper, std::string& data_message);

    /**
     * @brief Handles the end of data transmission.
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     */
    void HandleEndOfData(SocketWrapper& socket_wrapper);

    /**
     * @brief Handles the QUIT command.
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     */
    void HandleQuit(SocketWrapper& socket_wrapper);

    /**
     * @brief Handles the QUIT command in SSL/TCP mode.
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     */
    void HandleQuitSsl(SocketWrapper& socket_wrapper);
    void HandleQuitTcp(SocketWrapper& socket_wrapper);

    /**
     * @brief Handles the AUTH command.
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     * @param line The line of input containing the AUTH command.
     */
    void HandleAuth(SocketWrapper& socket_wrapper, const std::string& line);

    /**
     * @brief Handles the REGISTER command.
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     * @param line The line of input containing the REGISTER command.
     */
    void HandleRegister(SocketWrapper& socket_wrapper, const std::string& line);

    /**
     * @brief Decodes and splits the encoded data.
     * @param encoded_data The encoded data to decode and split.
     * @return A pair containing the decoded username and password.
     */
    static auto DecodeAndSplitPlain(const std::string& encoded_data) -> std::pair<std::string, std::string>;

    /**
     * @brief Saves a mail message to the database.
     * @param message The mail message to save.
     */
    void SaveMailToDatabase(const MailMessage& message);

    /**
     * @brief Connects to the mail database.
     */
    void ConnectToDatabase() const;

    /**
     * @brief Disconnects from the mail database.
     */
    void DisconnectFromDatabase() const;

private:
    boost::asio::ssl::context& m_ssl_context; ///< Reference to the SSL context.
    std::unique_ptr<ISXMailDB::PgMailDB> m_data_base; ///< Pointer to the mail database.
    MailMessageBuilder m_mail_builder; ///< Mail message builder instance.
    bool m_in_data = false; ///< Flag indicating if in DATA context.
    std::string m_connection_string = "postgresql://postgres.qotrdwfvknwbfrompcji:"
                                     "yUf73LWenSqd9Lt4@aws-0-eu-central-1.pooler."
                                     "supabase.com:6543/postgres?sslmode=require"; ///< Database connection string.
};
}

#endif // COMMANDHANDLER_H
