#include <boost/asio.hpp>

#include "Server.h"
#include "Logger.h"

using boost::asio::ip::tcp;
using namespace ISXSS;

int main() {
    try {
        boost::asio::io_context io_context;
        boost::asio::ssl::context ssl_context(boost::asio::ssl::context::tlsv12_server);

        // Load server certificates and private key
        ssl_context.use_certificate_chain_file("/etc/ssl/certs/smtp-server/server.crt");
        ssl_context.use_private_key_file("/etc/ssl/private/server.key", boost::asio::ssl::context::pem);

        SmtpServer server(io_context, ssl_context);
        server.Start();

        io_context.run();
    } catch (const std::exception& e) {
        Logger::LogError("Exception: " + std::string(e.what()));
    }
    return 0;
}