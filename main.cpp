#include <boost/asio.hpp>
#include <iostream>
#include "Server.h"

using boost::asio::ip::tcp;

int main() {
    try {
        boost::asio::io_context io_context;
        ThreadPool<> thread_pool; // Use default constructor for simplicity
        SmtpServer server(io_context, 2525, thread_pool);
        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
