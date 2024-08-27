#pragma once

#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <csignal>
#include <cstdlib>
#include <iostream>

#include "Logger.h"

namespace ISXSignalHandler
{
/**
 * @class SignalHandler
 * @brief A class that sets up and handles signal interruptions.
 *
 * This class provides static methods to set up signal handlers and
 * handle specific signals such.
 *
 * For now it is SIGIINT, SIGTERM, SIGSEGV, SIGABRT, SIGHUP
 */
class SignalHandler
{
public:
    /**
     * @brief Sets up signal handlers for the program.
     *
     * This method configures the signal handlers for different signals.
     */
    static void SetupSignalHandlers();

    /**
     * @brief Handles incoming signals.
     * @param[in] signal The signal number received.
     *
     * This method handles the specified signal.
     */
    static void HandleSignal(int signal);
};
}  // namespace ISXSignalHandler

#endif  // SIGNAL_HANDLER_H
