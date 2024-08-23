#include "SignalHandler.h"

namespace ISXSignalHandler
{
void SignalHandler::HandleSignal(const int signal)
{
    Logger::LogDebug("Entering SignalHandler::HandleSignal");
    Logger::LogTrace("SignalHandler::HandleSignal params: " + std::to_string(signal));

    switch (signal)
    {
        case SIGINT:
            Logger::LogDebug("Received SIGINT signal");
            Logger::LogProd("Program interrupted by user (SIGINT) with ^C. Exiting...");
            std::exit(EXIT_SUCCESS);
            break;

        case SIGTERM:
            Logger::LogDebug("Received SIGTERM signal");
            Logger::LogProd("Termination signal (SIGTERM) received. Exiting...");
            std::exit(EXIT_SUCCESS);
            break;

        case SIGSEGV:
            Logger::LogDebug("Received SIGSEGV signal");
            Logger::LogProd("Segmentation fault (SIGSEGV) occurred. Exiting...");
            std::exit(EXIT_FAILURE);
            break;

        case SIGABRT:
            Logger::LogDebug("Received SIGABRT signal");
            Logger::LogProd("Abort signal (SIGABRT) received. Exiting...");
            std::exit(EXIT_FAILURE);
            break;

#ifndef _WIN32
	case SIGHUP:
		Logger::LogDebug("Received SIGHUP signal");
		Logger::LogProd("Hangup signal (SIGHUP) received. Reinitializing...");
		break;
#endif
	default:
		Logger::LogDebug("Received unknown signal: " + std::to_string(signal));
		Logger::LogProd("Unknown signal (" + std::to_string(signal) + ") received. Exiting...");
		std::exit(EXIT_FAILURE);
		break;
	}
}

void SignalHandler::SetupSignalHandlers()
{
    Logger::LogDebug("Setting up signal handlers");

	std::signal(SIGINT, HandleSignal);
	std::signal(SIGTERM, HandleSignal);
	std::signal(SIGSEGV, HandleSignal);
	std::signal(SIGABRT, HandleSignal);
#ifndef _WIN32
	std::signal(SIGHUP, HandleSignal);
#endif

    Logger::LogDebug("Signal handlers setup complete");
}
}  // namespace ISXSignalHandler