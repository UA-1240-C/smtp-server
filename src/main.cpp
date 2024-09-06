#include <MailMessage.h>
#include <MailMessageBuilder.h>
#include <MailMessageForwarder.h>

#include <boost/asio.hpp>
#include <iostream>
#include <stdexcept>

#include "Logger.h"
#include "Server.h"

using boost::asio::ip::tcp;
using namespace ISXSS;

int main() {
    try {
        // Server setup
        boost::asio::io_context io_context;
        boost::asio::ssl::context ssl_context(boost::asio::ssl::context::tlsv12_server);

        // Load server certificates and private key
        ssl_context.use_certificate_chain_file("/etc/ssl/certs/smtp-server/server.crt");    // public key
        ssl_context.use_private_key_file("/etc/ssl/private/server.key", boost::asio::ssl::context::pem);

        SmtpServer server(io_context, ssl_context);
        server.Start();
/*
        // Test email forwarding
        std::string server_domain = "smtp.example.com";
        MailMessageForwarder forwarder(server_domain);

        // Create a test email message
        MailMessageBuilder mail_builder;
        mail_builder.set_from("user@gmail.com")
            .add_to("shevtsov.mykhail@gmail.com")
            .set_subject("Hello, Emma!")
            .set_body("Hello, Emma! This is a test email from John Doe.");

        MailMessage mail_message = mail_builder.Build();

        // Forward the email to the server
        bool success = forwarder.ForwardEmailToClientServer(mail_message);

        // Check the result
        if (success) {
            std::cout << "Test passed: Email successfully forwarded." << std::endl;
        } else {
            std::cout << "Test failed: Email not forwarded." << std::endl;
        }*/
        io_context.run();
    } catch (const std::exception& e) {
        Logger::LogError("Exception caught in entry point: " + std::string(e.what()));
    }

    return 0;
}
