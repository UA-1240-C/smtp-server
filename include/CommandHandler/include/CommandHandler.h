#pragma once

#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include <sodium/crypto_pwhash.h>

#include <boost/asio/ssl.hpp>
#include <string>

#include "Base64.h"
#include "MailDB/PgMailDB.h"
#include "MailMessageBuilder.h"
#include "SocketWrapper.h"

namespace ISXSC {
  class CommandHandler {
  public:
    CommandHandler(boost::asio::ssl::context& ssl_context);
    ~CommandHandler();

    void ProcessLine(const std::string& line, SocketWrapper& socket_wrapper);
  private:
    void HandleEhlo(SocketWrapper& socket_wrapper) ;
    void HandleMailFrom(SocketWrapper& socket_wrapper, const std::string& line);
    void HandleRcptTo(SocketWrapper& socket_wrapper, const std::string& line);

    void HandleData(SocketWrapper& socket_wrapper);
    void ReadData(SocketWrapper& socket_wrapper, std::string& data_message);
    void ProcessDataMessage(SocketWrapper& socket_wrapper, std::string& data_message);
    void HandleEndOfData(SocketWrapper& socket_wrapper);

    void HandleNoop(SocketWrapper& socket_wrapper);
    void HandleRset(SocketWrapper& socket_wrapper);
    void HandleHelp(SocketWrapper& socket_wrapper);
    void HandleVrfy(SocketWrapper& socket_wrapper, const std::string& line);
    void HandleExpn(SocketWrapper& socket_wrapper, const std::string& line);

    void HandleQuit(SocketWrapper& socket_wrapper);
    void HandleQuitSsl(SocketWrapper& socket_wrapper);
    void HandleQuitTcp(SocketWrapper& socket_wrapper);

    void HandleStartTLS(SocketWrapper& socket_wrapper);
    void HandleAuth(SocketWrapper& socket_wrapper, const std::string& line);
    void HandleRegister(SocketWrapper& socket_wrapper, const std::string& line);
    std::pair<std::string, std::string> DecodeAndSplitPlain(const std::string& encoded_data);

    bool VerifyPassword(const std::string& password,
                                    const std::string& hashed_password);
    std::string HashPassword(const std::string& password);
    void SaveMailToDatabase(const MailMessage& message);

    void ConnectToDatabase() const;
    void DisconnectFromDatabase() const;

    boost::asio::ssl::context& m_ssl_context;
    std::unique_ptr<ISXMailDB::PgMailDB> m_data_base;
    MailMessageBuilder m_mail_builder_;
    bool m_in_data_ = false;
    std::string m_connection_string = "postgresql://postgres.qotrdwfvknwbfrompcji:"
                                     "yUf73LWenSqd9Lt4@aws-0-eu-central-1.pooler."
                                     "supabase.com:6543/postgres?sslmode=require";
  };
}

#endif //COMMANDHANDLER_H
