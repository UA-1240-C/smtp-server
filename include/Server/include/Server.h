#pragma once

#ifndef SERVER_H
#define SERVER_H

#include <boost/asio.hpp>
#include <string>

#include "CommandHandler.h"
#include "MailDB/PgMailDB.h"
#include "MailMessageBuilder.h"
#include "SocketWrapper.h"
#include "ThreadPool.h"

using boost::asio::ip::tcp;

namespace ISXSC {

class SmtpServer {
public:
    SmtpServer(boost::asio::io_context& io_context, boost::asio::ssl::context& ssl_context);
    void Start();

private:
    std::string m_server_name;
    std::string m_server_display_name;
    unsigned int m_port;

    size_t m_max_threads;

    boost::asio::steady_timer m_timeout_timer_;
    std::chrono::seconds m_timeout_seconds_;

private:
    void Accept();
    void SaveData(const std::string& line, MailMessageBuilder& mail_builder, SocketWrapper& socket_wrapper,
                  bool& in_data);

private:
    MailMessageBuilder m_mail_builder_;
    ThreadPool<> m_thread_pool_;

    boost::asio::io_context &m_io_context_;
    boost::asio::ssl::context &m_ssl_context_;
    CommandHandler m_command_handler_;
    std::unique_ptr<tcp::acceptor> m_acceptor_;

    // std::string buffer_;

    // std::atomic_bool in_data_ = false;
    //  std::atomic_bool is_tls_ = false;

    // std::string current_sender_;
    // std::vector<std::string> current_recipients_;
private:
    void handleClient(SocketWrapper socket_wrapper);

    void tempHandleDataMode(const std::string& line, MailMessageBuilder& mail_builder, SocketWrapper& socket_wrapper,
                            bool& in_data);

    void resetTimeoutTimer(SocketWrapper& socket_wrapper);

private:
    void tempSaveMail(const MailMessage& message);
    void tempWriteEmailContent(std::ofstream& output_file, const MailMessage& message) const;
    void tempWriteAttachments(std::ofstream& output_file, const MailMessage& message) const;

    std::string tempCreateFileName() const;
    std::ofstream tempOpenFile(const std::string& fileName) const;

    bool tempIsOutputFileValid(const std::ofstream& output_file) const;

private:
    std::string hashPassword(const std::string& password);
    bool checkPassword(const std::string& stored_hash, const std::string& password);
};
}  // namespace ISXSC

#endif  // SERVER_H
