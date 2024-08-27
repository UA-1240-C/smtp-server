#pragma once

#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include <boost/asio/ssl.hpp>
#include <string>
#include <utility>
#include <openssl/ssl.h>
#include <openssl/evp.h>

#include "Base64.h"
#include "Logger.h"
#include "MailDB/PgMailDB.h"
#include "MailMessageBuilder.h"
#include "SocketWrapper.h"
#include "Logger.h"
#include "StandartSmtpResponses.h"

using namespace ISXMM;
using namespace ISXSocketWrapper;
using namespace ISXMailDB;
using namespace ISXBase64;

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
    explicit CommandHandler(boost::asio::ssl::context& ssl_context);

    /**
     * @brief Destructs the CommandHandler object.
     */
    ~CommandHandler();

    /**
     * @brief Processes a line of input from the client.
     * @param line The line of input to process.
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     */
    void ProcessLine(const std::string& line, SocketWrapper& socket_wrapper);

private:
    /**
     * @brief Handles the MAIL FROM command.
     *
     * The MAIL FROM command initiates a mail transfer. As an argument,
     * MAIL FROM includes a sender mailbox (reverse-path). For some
     * types of reporting messages like non-delivery notifications, the
     * reverse-path may be void. Optional parameters may also be specified.
     *
     * @code
     * MAIL FROM "test@client.net"
     * @endcode
     *
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     * @param line The line of input containing the MAIL FROM command.
     */
    void HandleMailFrom(SocketWrapper& socket_wrapper, const std::string& line);

    /**
     * @brief Handles the RCPT TO command.
     *
     * The RCPT TO command specifies the recipient. As an argument, RCPT TO
     * includes a destination mailbox (forward-path). In case of multiple
     * recipients, RCPT TO will be used to specify each recipient separately.
     *
     * An example of usage:
     * @code
     * RCPT TO "user@recipient.net"
     * @endcode
     *
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     * @param line The line of input containing the RCPT TO command.
     */
    void HandleRcptTo(SocketWrapper& socket_wrapper, const std::string& line);

    /**
     * @brief Handles the NOOP command.
     *
     * The NOOP command is used only to check whether the server can
     * respond. “250 OK” reply in response
     *
     * An example of usage:
     * @code
     * NOOP
     * @endcode
     *
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     */
    static void HandleNoop(SocketWrapper& socket_wrapper);

    /**
     * @brief Handles the RSET command.
     *
     * The RSET command resets the SMTP connection to the initial state. It
     * erases all the buffers and state tables (both sender and recipient).
     * RSET gets only the positive server response – 250. At the same time,
     * the SMTP connection remains open and is ready for a new mail transaction.
     *
     * An example of code usage:
     * @code
     * RSET
     * @endcode
     *
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     * @note The SMTP connection remains open and ready for a new transaction after this command.
     */
    void HandleRset(SocketWrapper& socket_wrapper);

    /**
     * @brief Handles the HELP command.
     *
     * With the HELP command, the client requests a list of commands the server
     * supports.
     *
     * An example of usage:
     * @code
     * HELP
     * @endcode
     *
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     */
    void HandleHelp(SocketWrapper& socket_wrapper);

    /**
     * @brief Handles the EHLO/HELO command.
     *
     * The HELO command initiates the SMTP session conversation.
     * The client greets the server and introduces itself. As a rule,
     * HELO is attributed with an argument that specifies the domain
     * name or IP address of the SMTP client.
     * In any case, HELO or EHLO is a MUST command for the SMTP
     * client to commence a mail transfer.
     *
     * @code
     * HELO client.net
     * @endcode
     *
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     */
    static void HandleEhlo(SocketWrapper& socket_wrapper);

    /**
     * @brief Handles the STARTTLS command.
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     */
    void HandleStartTLS(SocketWrapper& socket_wrapper);

    /**
     * @brief Handles the DATA command.
     *
     * With the DATA command, the client asks the server for permission to transfer
     * the mail data. The response code 354 grants permission, and the client launches
     * the delivery of the email contents line by line. This includes the date, from
     * header, subject line, to header, attachments, and body text.
     *
     * A final line containing a period (“.”) terminates the mail data transfer.
     * The server responses to the final line.
     *
     * @code
     *
     * DATA
     * 354 (server response code)
     * Date: Wed, 30 July 2019 06:04:34
     * From: test@client.net
     * Subject: How SMTP works
     * To: user@recipient.net
     * Body text
     * .
     * @endcode
     *
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
    void ProcessDataMessage(SocketWrapper& socket_wrapper, std::string& data_message, std::string& body);

    /**
     * @brief Handles the end of the data message.
     *
     * Sends a response to the client indicating the end of the data command.
     * It then attempts to build and save the mail message. If required fields are missing or
     * if an error occurs during saving, it sends an appropriate error response to the client.
     *
     * @param socket_wrapper Reference to the SocketWrapper instance used for managing the connection.
     */
    void HandleEndOfData(SocketWrapper& socket_wrapper);

    /**
     * @brief Handles the QUIT command.
     *
     * The QUIT command send the request to terminate the SMTP session. Once
     * the server responses with 221, the client closes the SMTP connection.
     * This command specifies that the receiver MUST send a “221 OK” reply and
     * then closes the transmission channel.
     *
     * An example of usage:
     * @code
     * QUIT
     * @endcode
     *
     * @see HandleQuitTcp()  // Reference to the method that might call HandleQuitTcp().
     * @see HandleQuitSsl()  // Self-reference for completeness in documentation.
     *
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     */
    void HandleQuit(SocketWrapper& socket_wrapper);

    /**
     * @brief Handles the AUTH command.
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     * @param line The line of input containing the AUTH command and user credentials.
     */
    void HandleAuth(SocketWrapper& socket_wrapper, const std::string& line);

    /**
     * @brief Handles the REGISTER command.
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     * @param line The line of input containing the REGISTER command and user credentials.
     */
    void HandleRegister(SocketWrapper& socket_wrapper, const std::string& line);

    /**
     * @brief Decodes Base64-encoded authentication data and splits it into username and password.
     *
     * @param encoded_data Base64-encoded authentication data in PLAIN format.
     * @return A pair containing the extracted username and password.
     * @throws std::runtime_error if the decoded data does not have the expected format (missing null bytes).
     */
    static auto DecodeAndSplitPlain(const std::string& encoded_data) -> std::pair<std::string, std::string>;

    /**
     * @brief Saves the provided mail message to the database.
     *
     * Attempts to insert the mail message into the database for each recipient.
     * Logs any errors encountered during the process.
     *
     * @param message The MailMessage object containing the details of the mail to be saved.
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

    std::vector<unsigned char> ExtractSessionKey(SslSocket& ssl_socket);
    std::string ComputeHMAC(const std::vector<unsigned char>& key, const std::string& data);
private:
    boost::asio::ssl::context& m_ssl_context;  ///< Reference to the SSL context.
    std::unique_ptr<PgMailDB> m_data_base;     ///< Pointer to the mail database.
    MailMessageBuilder m_mail_builder;         ///< Mail message builder instance.
    bool m_in_data = false;                    ///< Flag indicating if in DATA context.
    std::string m_connection_string =
        "postgresql://postgres.qotrdwfvknwbfrompcji:"
        "yUf73LWenSqd9Lt4@aws-0-eu-central-1.pooler."
        "supabase.com:6543/postgres?sslmode=require";  ///< Database connection string.
};
}  // namespace ISXCommandHandler

#endif  // COMMANDHANDLER_H
