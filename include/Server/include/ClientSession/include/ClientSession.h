#ifndef ClientState_H
#define ClientState_H

#include <boost/asio.hpp>
#include <memory>
#include "SmtpRequest.h"
#include "SocketWrapper.h"

using boost::asio::ip::tcp;
using std::unique_ptr;

using ISXSmtpRequest::SmtpRequest;
using ISXSmtpRequest::RequestParser;

using ISXSocketWrapper::SocketWrapper;
using TcpSocket = boost::asio::ip::tcp::socket;
using TcpSocketPtr = std::shared_ptr<TcpSocket>;

namespace ISXCState {
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

    void Greet();
    void PollForRequest();

private:
    void HandleNewRequest();
    void ProcessRequest(const SmtpRequest& request);

    void HandleStaticCommands(const SmtpRequest& request);

    void HandleConnectedState(const SmtpRequest& request);
    void HandleEhloSentState(const SmtpRequest& request);

    ClientState m_current_state;
    SocketWrapper m_socket;
    boost::asio::ssl::context* m_ssl_context;

    std::chrono::seconds m_timeout_duration;
};
}; // namespace ISXCState

#endif // !ClientState_H
