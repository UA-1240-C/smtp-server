#ifndef SERVER_H
#define SERVER_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <crypt.h>
#include <string>
#include <vector>

#include "MailMessageBuilder.h"
#include "ThreadPool.h"
#include "SocketWrapper.h"
#include "Base64.h"

using boost::asio::ip::tcp;

namespace ISXSC
{
    struct MailData {
        std::string sender_;
        std::vector<std::string> recipients_;
        std::string message_;
    };

    class SmtpServer {
    public:
        SmtpServer(boost::asio::io_context& io_context,
            boost::asio::ssl::context& ssl_context,
            unsigned short port,
            ThreadPool<>& thread_pool);

        void Start();
    private:
        void Accept();
        void handleClient(SocketWrapper socket_wrapper);

        void saveMail(const MailMessage& message);

        void saveData(const std::string& line,
                        MailMessageBuilder& mail_builder,
                        SocketWrapper socket_wrapper,
                        bool& in_data);

        void upgradeToTLS(std::shared_ptr<tcp::socket> socket);


        void sendResponse(SocketWrapper socket_wrapper, const std::string& response);

        void handle_HELO(SocketWrapper socket_wrapper);
        void handle_MAIL_FROM(SocketWrapper socket_wrapper, const std::string& line);
        void handle_RCPT_TO(SocketWrapper socket_wrapper, const std::string& line);
        void handle_DATA(SocketWrapper socket_wrapper);
        void handle_QUIT(SocketWrapper socket_wrapper);
        void handle_NOOP(SocketWrapper socket_wrapper);
        void handle_RSET(SocketWrapper socket_wrapper);
        void handle_HELP(SocketWrapper socket_wrapper);
        void handle_STARTTLS(SocketWrapper socket_wrapper);
        void handle_AUTH(SocketWrapper socket_wrapper, const std::string& line);

        std::string hashPassword(const std::string& password);
        bool validateCredentials(const std::string& username, const std::string& password);
    private:
        boost::asio::io_context& io_context_;
        boost::asio::ssl::context& ssl_context_;
        std::unique_ptr<tcp::acceptor> acceptor_;
        ThreadPool<>& thread_pool_;
        MailMessageBuilder mail_builder_;
        std::string buffer_;


        bool in_data_ = false;
        bool is_tls = false;
    };
}

#endif // SERVER_H
