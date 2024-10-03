#ifndef ClientState_H
#define ClientState_H

#include <boost/asio.hpp>
#include <memory>

#include "SmtpRequest.h"
#include "SocketWrapper.h"
#include "MailDB/PgMailDB.h"
#include "MailMessageBuilder.h"

using boost::asio::ip::tcp;
using std::unique_ptr;

using ISXSmtpRequest::SmtpRequest;
using ISXSmtpRequest::RequestParser;
using ISXSocketWrapper::SocketWrapper;

using ISXMailDB::PgMailDB;
using ISXMM::MailMessageBuilder;

using TcpSocket = boost::asio::ip::tcp::socket;
using TcpSocketPtr = std::shared_ptr<TcpSocket>;

namespace ISXCState {

constexpr std::size_t USERNAME_START_INDEX = 5;
constexpr std::size_t RECIPIENT_START_INDEX = 8;
constexpr std::size_t AUTH_PREFIX_LENGTH = 11;
constexpr std::size_t REGISTER_PREFIX_LENGTH = 9;

enum class ClientState {
    CONNECTED,
    EHLO_SENT,
    STARTTLS_SENT,
    AUTH_SENT,
    MAILFROM_SENT,
    RCPTTO_SENT,
    DATA_SENT,
    RSET_SENT,
    QUIT_SENT,
};

class ClientSession {
public:
    ClientSession(TcpSocketPtr socket
                  , boost::asio::ssl::context& ssl_context
                  , std::chrono::seconds timeout_duration);

    ~ClientSession();

    void Greet();
    void PollForRequest();

private:
    void HandleNewRequest();
    void ProcessRequest(const SmtpRequest& request);

    // Command handlers
    bool HandleStaticCommands(const SmtpRequest& request);
    void HandleRegister(const SmtpRequest& request);
    void HandleAuth(const SmtpRequest& request);
    void HadleStartTls(const SmtpRequest& request);
    void HandleMailFrom(const SmtpRequest& request);
    void HandleRcptTo(const SmtpRequest& request);
    void HandleData(const SmtpRequest& request);
    void HandleRset(const SmtpRequest& request);

    // State handlers
    void HandleConnectedState(const SmtpRequest& request);
    void HandleEhloSentState(const SmtpRequest& request);
    void HandleStartTlsSentState(const SmtpRequest& request);
    void HandleAuthSentState(const SmtpRequest& request);
    void HandleMailFromSentState(const SmtpRequest& request);
    void HandleRcptToSentState(const SmtpRequest& request);

    // Utility functions
    std::string ReadDataUntilEOM();
    void ConnectToDataBase();
    void BuildMessageData(const std::string& data);
    void SaveMessageToDataBase(MailMessage& message);

    ClientState m_current_state;
    SocketWrapper m_socket;
    boost::asio::ssl::context* m_ssl_context;

    std::chrono::seconds m_timeout_duration;

    std::unique_ptr<PgMailDB> m_data_base;     ///< Pointer to the mail database for storing and retrieving mail messages.
    MailMessageBuilder m_mail_builder;         ///< Instance of the mail message builder for constructing messages.
    std::string m_connection_string =
        "postgresql://postgres.qotrdwfvknwbfrompcji:"
        "yUf73LWenSqd9Lt4@aws-0-eu-central-1.pooler."
        "supabase.com:6543/postgres?sslmode=require";  ///< Data base connection string.i
};
}; // namespace ISXCState

#endif // !ClientState_H
