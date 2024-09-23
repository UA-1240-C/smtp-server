#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include <string>
#include <utility>

#include <boost/asio/ssl.hpp>

#include "Base64.h"
#include "MailDB/PgMailDB.h"
#include "MailDB/ConnectionPool.h"
#include "MailDB/PgManager.h"
#include "MailMessageBuilder.h"
#include "SocketWrapper.h"

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
    *
    * This constructor initializes the CommandHandler with a reference to an SSL context
    * and establishes a connection to the PostgreSQL mail database. SSL options are set to
    * disable older, less secure protocols.
    *
    * @param io_context A reference to the boost::asio::io_context used for asynchronous operations management.
    * @param connection_pool A reference to the ISXMailDB::ConnectionPool used for database connections.
    * @param ssl_context A reference to the boost::asio::ssl::context used for SSL connections.
    *
    * @exception MailException Thrown if there is an error during the database connection.
    * @see DataBase::MailDB::include::MailDB::MailException.h
    */
    explicit CommandHandler(boost::asio::io_context& io_context, boost::asio::ssl::context& ssl_context,
                            ISXMailDB::PgManager& database_manager);
    /**
     * @brief Destructs the CommandHandler object and disconnects from the data base.
     */
    ~CommandHandler();

     /**
     * @brief Processes an incoming SMTP command line.
     *
     * This function analyzes the provided command line and delegates the processing to the appropriate
     * handler based on the command. If the command is not recognized, it sends a syntax error response
     * back to the client.
     *
     * @param line The incoming SMTP command line to be processed.
     * @param socket_wrapper A reference to the `SocketWrapper` used for sending responses to the client.
     *
     * @exception std::invalid_argument Thrown if an invalid argument is encountered while sending a response.
     *
     * The function handles various SMTP commands, including:
     * - `EHLO` or `HELO`: Handled by `HandleEhlo()`.
     * - `MAIL FROM:`: Handled by `HandleMailFrom()`.
     * - `RCPT TO:`: Handled by `HandleRcptTo()`.
     * - `DATA`: Handled by `HandleData()`.
     * - `QUIT`: Handled by `HandleQuit()`.
     * - `NOOP`: Handled by `HandleNoop()`.
     * - `RSET`: Handled by `HandleRset()`.
     * - `HELP`: Handled by `HandleHelp()`.
     * - `STARTTLS`: Handled by `HandleStartTLS()`.
     * - `AUTH PLAIN`: Handled by `HandleAuth()`.
     * - `REGISTER`: Handled by `HandleRegister()`.
     * - Unrecognized commands: Responds with a syntax error.
     */
    void ProcessLine(const std::string& line, SocketWrapper& socket_wrapper);

private:
    /**
     * @brief Handles the MAIL FROM command.
     *
     * This function processes the MAIL FROM command, which specifies the sender's email address. It checks
     * whether the sender exists in the database and, if valid, sets the sender address in the mail builder.
     *
     * An example of the usage: @code MAIL FROM: <test@client.net>@endcode
     *
     * @param socket_wrapper Reference to the `SocketWrapper` object used for communication with the client.
     * @param line The full command line received from the client, containing the sender's address.
     */
    void HandleMailFrom(SocketWrapper& socket_wrapper, const std::string& line);

    /**
     * @brief Handles the RCPT TO command.
     *
     * This function processes the RCPT TO command, which specifies the recipient's email address. It checks
     * whether the recipient exists in the database and, if valid, adds the recipient to the mail builder.
     *
     * An example of the usage: @code RCPT TO: <user@recipient.net>@endcode
     *
     * @param socket_wrapper Reference to the `SocketWrapper` object used for communication with the client.
     * @param line The full command line received from the client, containing the recipient's address.
     */
    void HandleRcptTo(SocketWrapper& socket_wrapper, const std::string& line);

    /**
     * @brief Handles the NOOP command.
     *
     * The NOOP command is used only to check whether the server can
     * respond. “250 OK” reply in response
     *
     * An example of the usage: @code NOOP@endcode
     *
     * @param socket_wrapper Reference to the `SocketWrapper` object used for communication with the client.
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
     * An example of the usage: @code RSET@endcode
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
     * For now server recognizes various SMTP commands, including:
     * - `EHLO` or `HELO`.
     * - `MAIL FROM:`.
     * - `RCPT TO:`:.
     * - `DATA`.
     * - `QUIT`.
     * - `NOOP`.
     * - `RSET`.
     * - `HELP`.
     * - `STARTTLS`.
     * - `AUTH PLAIN`.
     * - `REGISTER`.
     *
     * An example of the usage: @code HELP@endcode
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
     *
     * An example of the usage: @code HELO client.net@endcode
     *
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     */
    static void HandleEhlo(SocketWrapper& socket_wrapper);

    /**
     * @brief Handles the access token received over the socket.
     *
     * This function processes a line of input, extracts the access token from the line, 
     * and sends a success response to the client. It uses asynchronous writing to 
     * communicate with the client and logs the token for debugging purposes.
     * 
     * @param socket_wrapper Reference to a SocketWrapper object used to send and receive data asynchronously.
     * @param line The string containing the access token information. The access token is extracted from this line.
     * 
     * @exception std::exception If an error occurs during socket communication, it is caught and logged as an error.
     * 
     * @note The access token is assumed to start at position 13 of the input line.
     */
    void HandleAccessToken(SocketWrapper& socket_wrapper, std::string line);

    /**
     * @brief Handles the STARTTLS command by initiating the TLS handshake if not already in TLS mode.
     *
     * This function checks if the connection is already in TLS mode. If it is, it sends an SMTP response indicating
     * that the STARTTLS command was issued out of sequence. If not, it sends an OK response, and then proceeds
     * to perform the TLS handshake, upgrading the connection to TLS.
     *
     * @param socket_wrapper Reference to a SocketWrapper object used for network communication.
     *
     * @exception std::exception Throws if an error occurs during network communication or TLS handshake.
     *
     * @note The function ensures that the STARTTLS command is only executed when not already in TLS mode.
     *       It logs various states and handles exceptions during the process.
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
     * An example of the usage: @code
     * DATA
     * 354 (server response code)
     * From: test@client.net
     * To: user@gmail.com
     * Subject: How SMTP works
     * Body text
     * .
     * @endcode
     *
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     */
    void HandleData(SocketWrapper& socket_wrapper);

    /**
     * @brief Reads data from the socket during the DATA command.
     *
     * This function reads the incoming data from the client during the DATA command and appends it to the
     * `data_message` string.
     *
     * @param socket_wrapper Reference to the `SocketWrapper` object used for communication with the client.
     * @param data_message Reference to a string where the incoming data is appended.
     */
    void ReadData(SocketWrapper& socket_wrapper, std::string& data_message);

    /**
     * @brief Processes the email data received from the client during the DATA command.
     *
     * This function is responsible for parsing and processing the email content sent by the client after the
     * DATA command has been issued. It reads the incoming data, extracts the email subject and body, and
     * detects the end-of-data sequence which indicates the completion of the email content transmission.
     *
     * The function performs the following steps:
     * - Extracts individual lines from the received data.
     * - Identifies the end-of-data sequence (a single dot on a line by itself) and processes the email content accordingly.
     * - Handles specific lines starting with "Subject: " to set the email subject.
     * - Accumulates lines to form the email body.
     *
     * If an exception is thrown during processing, it is logged and propagated to the caller.
     *
     * @param socket_wrapper Reference to the `SocketWrapper` object used for communication with the client.
     * @param data_message Reference to a string containing the data received from the client. This string is
     *                     updated with the received data and processed to extract email content.
     *
     * @throws std::exception If an error occurs while processing the data.
     *
     * @details The function uses `data_message` to accumulate the data read from the socket. It processes the
     *          data in chunks, searching for line breaks to identify individual lines. Each line is checked
     *          to determine if it starts with "Subject: " or if it signifies the end of the email content.
     *          The body of the email is built up from the remaining lines. Once the end-of-data sequence is
     *          detected, the function completes the email body and triggers further processing.
     */
    void ProcessDataMessage(SocketWrapper& socket_wrapper, std::string& data_message, std::string& body);

    /**
     * @brief Handles the end of the DATA command.
     *
     * This function finalizes the email after the end-of-data sequence is detected. It checks if the required
     * fields are present and, if so, saves the email to the database. The `MailMessageBuilder` is then reset
     * for the next email.
     *
     * @param socket_wrapper Reference to the `SocketWrapper` object used for communication with the client.
     * @see SaveMailToDatabase(const MailMessage& message)
     */
    void HandleEndOfData(SocketWrapper& socket_wrapper);

    /**
     * @brief Forwards an email using the provided OAuth2 token for authentication.
     *
     * This function connects to the SMTP server via raw TCP socket, encrypts connection
     * using STARTTLS and authenticates a client initiated a message sending using the 
     * XOAUTH2 command with the provided OAuth2 token. If authentication is
     * successful, the email message is sent to the recipients.
     *
     * @param message The email message to be sent, including subject, body, and recipients.
     * @param oauth2_token The OAuth2 token used for XOAUTH2 authentication.
     *
     * @throws boost::system::system_error Thrown if there is an error in the underlying
     * network operations or SSL handshake.
     * @throws std::runtime_error Thrown if authentication or communication with the SMTP
     * server fails.
     */
    void ForwardMail(const MailMessage& message, const std::string& oauth2_token);
 
   /**
     * @brief Handles the QUIT command.
     *
     * The QUIT command send the request to terminate the SMTP session. Once
     * the server responses with 221, the client closes the closes a socket depending on its type.
     * This command specifies that the receiver MUST send a “221 OK” reply and
     * then closes the transmission channel.
     *
     * An example of usage: @code QUIT@endcode
     *
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     */
    void HandleQuit(SocketWrapper& socket_wrapper);

    /**
     * @brief Handles the AUTH command.
     *
     * The AUTH command is used to authenticate the client using a specified authentication mechanism. This method supports
     * PLAIN authentication mechanism: @code user@gmail.com\0password\0 @endcode.
     * It validates the provided credentials and, if successful,
     * allows the client to proceed with sending mail. If authentication fails, an error message is sent to the client.
     *
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     * @param line The line of input containing the AUTH command and user credentials.
     */
    void HandleAuth(SocketWrapper& socket_wrapper, const std::string& line);

    /**
     * @brief Handles the custom REGISTER command.
     *
     * The REGISTER command is used to registrate the client using a specified mechanism. This method supports
     * PLAIN authentication mechanism: @code user@gmail.com\0password\0 @endcode. It writes the provided credentials
     * and, if successful, allows the client to register and authenticate soon. If registration fails, an error message
     * is sent to the client.
     *
     * @param socket_wrapper Reference to the SocketWrapper for communication.
     * @param line The line of input containing the REGISTER command and user credentials.
     */
    void HandleRegister(SocketWrapper& socket_wrapper, const std::string& line);

    /**
     * @brief Decodes Base64-encoded authentication data in PLAIN format and splits it into username and password.
     *
     * @param encoded_data Base64-encoded authentication data in PLAIN format.
     * @return A pair containing the extracted username and password.
     * @throws std::runtime_error if the decoded data does not have the expected format (missing null bytes).
     */
    static auto DecodeAndSplitPlain(const std::string& encoded_data) -> std::pair<std::string, std::string>;

   /**
    * @brief Saves the given mail message to the database.
    *
    * This function iterates over the recipients of the provided mail message and attempts to insert
    * each email into the database. It logs the process, including entering the function, the parameters,
    * and the success or failure of each email insertion.
    *
    * The function is closely related to the `InsertMail` function found in
    * `DataBase::MailDB::include::MailDB::PgMailDB`. The `InsertMail` function is responsible for the actual
    * insertion of the email data into the database.
    *
    * @param message The mail message that needs to be saved to the database. The message includes the sender's address,
    *                the list of recipients, the subject, and the body of the email.
    *
    * @see DataBase::MailDB::include::MailDB::PgMailDB::InsertMail
    */
    void SaveMailToDatabase(const MailMessage& message);

    /**
   * @brief Establishes a connection to the mail database.
   *
   * This function is responsible for connecting to the mail database, enabling subsequent database operations.
   * It is closely related to the `Connect` function found in `DataBase::MailDB::include::MailDB::PgMailDB`,
   * which handles the actual connection logic.
   *
   * @see DataBase::MailDB::include::MailDB::PgMailDB::Connect
   */
    void ConnectToDatabase() const;

    /**
     * @brief Closes the connection to the mail database.
     *
     * This function handles disconnecting from the mail database, ensuring that the connection is properly closed.
     * It is closely related to the `Disconnect` function located in `DataBase::MailDB::include::MailDB::PgMailDB`,
     * which performs the actual disconnection process.
     *
     * @see DataBase::MailDB::include::MailDB::PgMailDB::Disconnect
     */
    void DisconnectFromDatabase() const;

    /**
     * @brief Asynchronously connects to the SMTP server and initiates the SSL handshake.
     *
     * This function asynchronously resolves and connects to the SMTP server(for now, at smtp.gmail.com on port 587.
     * Once connected, it reads the initial server response and sends the HELO command to begin the SSL handshake.
     *
     * @param socket_wrapper Reference to a SocketWrapper object used to send and receive data asynchronously over the network.
     *
     * @exception std::exception Throws if any error occurs during connection, writing, or reading from the socket.
     *
     * @note This function specifically connects to smtp.gmail.com and sends the HELO command.
     */
    void ConnectToSmtpServer(SocketWrapper& socket_wrapper);
    
    /**
     * @brief Sends a command through the socket and returns the server's response.
     * 
     * This function asynchronously sends a given SMTP command to the server, followed by a carriage return
     * and newline, and waits for the server's response. The response is then returned as a string.
     * 
     * @param socket_wrapper Reference to a SocketWrapper object used to send and receive data asynchronously.
     * @param command The SMTP command to send to the server (e.g., "HELO", "MAIL FROM").
     * 
     * @return std::string The server's response to the sent command.
     * 
     * @exception std::exception Throws if any error occurs during writing or reading from the socket.
     * 
     * @note The command is automatically appended with "\r\n" before being sent to the server.
     */
    std::string SendSmtpCommand(SocketWrapper& socket_wrapper, const std::string& command);
private:
    boost::asio::io_context& m_io_context;     ///< Reference to the IO context for async operations handling.
    boost::asio::ssl::context& m_ssl_context;  ///< Reference to the SSL context for secure connections.
    std::unique_ptr<PgMailDB> m_data_base;     ///< Pointer to the mail database for storing and retrieving mail messages.
    MailMessageBuilder m_mail_builder;         ///< Instance of the mail message builder for constructing messages.
    bool m_in_data = false;                    ///< lag indicating whether the server is currently processing mail data.
    std::string m_access_token;                ///< An access token used to authenticate sender on its mail exchange server
};
}  // namespace ISXCommandHandler

#endif  // COMMANDHANDLER_H
