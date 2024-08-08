#pragma once

#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include <string>
#include <exception>
#include "iostream"

#include <boost/system/error_code.hpp>

#include "SocketWrapper.h"

using namespace ISXSocketWrapper;

namespace ISXErrorHandler {
/**
 * @struct ErrorHandler
 * @brief Provides static methods for handling errors in the application.
 *
 * The ErrorHandler struct contains static methods to handle various types of
 * errors, including Boost error codes and exceptions. It provides functionality
 * to handle errors by logging them or responding to clients.
 */
struct ErrorHandler {
  /**
   * @brief Handles a Boost error code.
   * @param where The context or location where the error occurred.
   * @param error The Boost error code.
   *
   * This method logs the Boost error code and its associated message,
   * providing information about the context where the error occurred.
   */
  static void HandleBoostError(const std::string& where, const boost::system::error_code& error);

  /**
   * @brief Handles an exception.
   * @param where The context or location where the exception occurred.
   * @param e The exception to handle.
   *
   * This method logs the exception and its message, providing information
   * about the context where the exception occurred.
   */
  static void HandleException(const std::string& where, const std::exception& e);

  /**
   * @brief Handles a generic error and responds to the client.
   * @param context The context or location where the error occurred.
   * @param e The exception that represents the error.
   * @param socket_wrapper Reference to the SocketWrapper for communication.
   * @param error_response The error response to send to the client.
   *
   * This method handles the error by logging it and sending an appropriate
   * error response to the client using the provided SocketWrapper.
   */
  static void HandleError(const std::string& context,
      const std::exception& e,
      SocketWrapper& socket_wrapper,
      const std::string& error_response);
};

inline void ErrorHandler::HandleBoostError(const std::string& where, const boost::system::error_code& error)
{
  std::cerr << where << " error: " << error.message() << std::endl;
}

inline void ErrorHandler::HandleException(const std::string& where, const std::exception& e)
{
  std::cerr << where << " error: " << e.what() << std::endl;
}

inline void ErrorHandler::HandleError(const std::string& context,
    const std::exception& e,
    SocketWrapper& socket_wrapper,
    const std::string& error_response)
{
  std::cerr << context << " error: " << e.what() << std::endl;
  socket_wrapper.SendResponseAsync(error_response + "\r\n").get();
}
}

#endif //ERRORHANDLER_H