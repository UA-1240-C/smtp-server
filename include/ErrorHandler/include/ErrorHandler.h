#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include <string>
#include <boost/system/error_code.hpp>
#include <exception>
#include "iostream"

#include "SocketWrapper.h"

namespace ISXSC {
  struct ErrorHandler {
    static void handleBoostError(const std::string& where, const boost::system::error_code& error);

    static void handleException(const std::string& where, const std::exception& e);

    static void handleError(const std::string& context, const std::exception& e, SocketWrapper& socket_wrapper, const std::string& error_response);
};

  inline void ErrorHandler::handleBoostError(const std::string& where, const boost::system::error_code& error)
  {
    std::cerr << where << " error: " << error.message() << std::endl;
  }

  inline void ErrorHandler::handleException(const std::string& where, const std::exception& e)
  {
    std::cerr << where << " error: " << e.what() << "\n";
  }

  inline void ErrorHandler::handleError(const std::string& context, const std::exception& e,
    SocketWrapper& socket_wrapper, const std::string& error_response)
  {
    std::cerr << context << " error: " << e.what() << std::endl;
    auto future = socket_wrapper.SendResponseAsync(error_response + "\r\n");
    try {
      future.get(); // Ожидание завершения асинхронной операции
      std::cout << "Response sent successfully." << std::endl;
    } catch (const std::exception& e) {
      std::cerr << "Error sending response: " << e.what() << std::endl;
    }
  }
}

#endif //ERRORHANDLER_H