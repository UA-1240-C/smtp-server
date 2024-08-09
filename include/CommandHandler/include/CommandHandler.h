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

    void processLine(const std::string& line, SocketWrapper& socket_wrapper,
                     bool in_data, MailMessageBuilder& mail_builder);
  private:
    void handleEhlo(SocketWrapper& socket_wrapper) ;
    void handleMailFrom(SocketWrapper& socket_wrapper, const std::string& line);
    void handleRcptTo(SocketWrapper& socket_wrapper, const std::string& line);

    void handleData(SocketWrapper& socket_wrapper);
    void readData(SocketWrapper& socket_wrapper, std::string& data_message);
    void processDataMessage(SocketWrapper& socket_wrapper, std::string& data_message);
    void handleEndOfData(SocketWrapper& socket_wrapper);

    void handleNoop(SocketWrapper& socket_wrapper);
    void handleRset(SocketWrapper& socket_wrapper);
    void handleHelp(SocketWrapper& socket_wrapper);
    void handleVrfy(SocketWrapper& socket_wrapper, const std::string& line);
    void handleExpn(SocketWrapper& socket_wrapper, const std::string& line);

    void handleQuit(SocketWrapper& socket_wrapper);
    void handleQuitSsl(SocketWrapper& socket_wrapper);
    void handleQuitTcp(SocketWrapper& socket_wrapper);

    void handleStartTLS(SocketWrapper& socket_wrapper);
    void handleAuth(SocketWrapper& socket_wrapper, const std::string& line);
    void handleRegister(SocketWrapper& socket_wrapper, const std::string& line);
    std::pair<std::string, std::string> decodeAndSplitPlain(const std::string& encoded_data);

    bool verifyPassword(const std::string& password,
                                    const std::string& hashed_password);
    std::string hashPassword(const std::string& password);
    void saveMailToDatabase(const MailMessage& message);

    void ConnectToDatabase() const;
    void DisconnectFromDatabase() const;

    boost::asio::ssl::context& ssl_context_;
    std::unique_ptr<ISXMailDB::PgMailDB> data_base_;
    MailMessageBuilder mail_builder_;
    bool in_data_ = false;
    std::string connection_string_ = "postgresql://postgres.qotrdwfvknwbfrompcji:"
                                     "yUf73LWenSqd9Lt4@aws-0-eu-central-1.pooler."
                                     "supabase.com:6543/postgres?sslmode=require";
  };
}

#endif //COMMANDHANDLER_H
