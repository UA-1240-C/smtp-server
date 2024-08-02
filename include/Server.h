#ifndef SERVER_H
#define SERVER_H

#include <crypt.h>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <string>
#include <vector>

#include "Base64.h"
#include "MailMessageBuilder.h"
#include "SocketWrapper.h"
#include "ThreadPool.h"
#include "DataBase/DataBase-main/libs/MailDB/include/MailDB/PgMailDB.h"
// #include "DataBase/DataBase-main/libs/MailDB/include/MailDB/MailException.h"

using boost::asio::ip::tcp;

namespace ISXSC {
struct MailData {
    std::string sender_;
    std::string message_;

    std::vector<std::string> recipients_;
};

class SmtpServer {
public:
    SmtpServer(boost::asio::io_context& io_context,
               boost::asio::ssl::context& ssl_context, unsigned short port,
               ThreadPool<>& thread_pool);
    ~SmtpServer();
    void Start();

private:
    void Accept();
    void saveData(const std::string& line, MailMessageBuilder& mail_builder,
                  SocketWrapper socket_wrapper, bool& in_data);

    void upgradeToTLS(std::shared_ptr<tcp::socket> socket);

    void sendResponse(SocketWrapper socket_wrapper,
                      const std::string& response);

    void handleEhlo(SocketWrapper socket_wrapper);
    void handleMailFrom(SocketWrapper socket_wrapper, const std::string& line);
    void handleRcptTo(SocketWrapper socket_wrapper, const std::string& line);
    void handleNoop(SocketWrapper socket_wrapper);
    void handleRset(SocketWrapper socket_wrapper);
    void handleHelp(SocketWrapper socket_wrapper);
    void handleStartTLS(SocketWrapper socket_wrapper);
    void handleAuth(SocketWrapper socket_wrapper, const std::string& line);
    void handleRegister(SocketWrapper socket_wrapper, const std::string& line);


private:
    // Should be implemented Hash/Validate
    /*
        std::string hashPassword(const std::string& password);
        bool validateCredentials(const std::string& username,
                             const std::string& password);
    */

private:
    MailMessageBuilder mail_builder_;
    ThreadPool<>& thread_pool_;

    boost::asio::io_context& io_context_;
    boost::asio::ssl::context& ssl_context_;

    std::unique_ptr<tcp::acceptor> acceptor_;

    std::string buffer_;

    std::atomic_bool in_data_ = false;
    std::atomic_bool is_tls_ = false;

    std::unique_ptr<ISXMailDB::PgMailDB> data_base_;
    std::string connection_string_ = "postgresql://postgres.qotrdwfvknwbfrompcji:yUf73LWenSqd9Lt4@aws-0-eu-central-1.pooler.supabase.com:6543/postgres?sslmode=require";

    std::string current_sender_;
    std::vector<std::string> current_recipients_;
private:
    void handleClient(SocketWrapper socket_wrapper);
    void readFromSocket(SocketWrapper& socket_wrapper,
                        std::array<char, 1024>& buffer, size_t& length,
                        boost::system::error_code& error);

    void processLine(const std::string& line, SocketWrapper& socket_wrapper,
                     bool& in_data, MailMessageBuilder& mail_builder);

    void tempHandleDataMode(const std::string& line,
                            MailMessageBuilder& mail_builder,
                            SocketWrapper& socket_wrapper, bool& in_data);

    void handleBoostError(std::string where,
                          const boost::system::error_code& error) const;

    void handleException(std::string where, const std::exception& e) const;

private:
    void tempSaveMail(const MailMessage& message);
    void tempWriteEmailContent(std::ofstream& output_file,
                               const MailMessage& message) const;
    void tempWriteAttachments(std::ofstream& output_file,
                              const MailMessage& message) const;

    std::string tempCreateFileName() const;
    std::ofstream tempOpenFile(const std::string& fileName) const;

    bool tempIsOutputFileValid(const std::ofstream& output_file) const;

private:
    void handleQuit(SocketWrapper socket_wrapper);
    void handleQuitSsl(SocketWrapper socket_wrapper);
    void handleQuitTcp(SocketWrapper socket_wrapper);

private:
    void handleData(SocketWrapper socket_wrapper);
    void readData(SocketWrapper& socket_wrapper, std::string& data_message);
    void processDataMessage(std::string& data_message,
                            SocketWrapper& socket_wrapper);
    void handleEndOfData(SocketWrapper& socket_wrapper);

private:
    void ConnectToDatabase();
    void DisconnectFromDatabase();
    void SaveMailToDatabase(const MailMessage& message);
    std::string hashPassword(const std::string& password);
    bool checkPassword(const std::string& stored_hash, const std::string& password);
};
}  // namespace ISXSCІ

#endif  // SERVER_H
