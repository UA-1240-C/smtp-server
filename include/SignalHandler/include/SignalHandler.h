#pragma once

#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <csignal>
#include <cstdlib>
#include <iostream>

namespace ISXSignalHandler
{
/**
 * @class SignalHandler
 * @brief A class that sets up and handles signal interruptions.
 *
 * This class provides static methods to set up signal handlers and
 * handle specific signals such as SIGINT (interrupt signal).
 */
class SignalHandler
{
public:
	/**
		* @brief Sets up signal handlers for the program.
		*
		* This method configures the signal handlers for different signals.
		* Currently, it sets up handling for SIGINT (interrupt signal).
		*/
	static void SetupSignalHandlers();

	/**
	 * @brief Handles incoming signals.
	 * @param[in] signal The signal number received.
	 *
	 * This method handles the specified signal. For SIGINT, it prints
	 * a message to standard error and exits the program with a success
	 * status code.
	 */
	static void HandleSignal(int signal);
};
}

#endif  // SIGNAL_HANDLER_H
