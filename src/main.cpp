#include <iostream>

#include <boost/asio.hpp>

#include "Server.h"
#include "SignalHandler.h"

using boost::asio::ip::tcp;
using namespace ISXSS;

int main() {
    SignalHandler::SetupSignalHandlers();

    try {
        boost::asio::io_context io_context;
        boost::asio::ssl::context ssl_context(boost::asio::ssl::context::tlsv12_server);

        // Load server certificates and private key
        ssl_context.use_certificate_chain_file("../server.crt");
        ssl_context.use_private_key_file("../server.key", boost::asio::ssl::context::pem);

        std::cout << "SSL context set up with certificates." << std::endl;

        SmtpServer server(io_context, ssl_context);
        server.Start();

        std::cout << "Server is running. Press Ctrl+C to stop." << std::endl;

        io_context.run();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}
