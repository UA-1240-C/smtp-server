#include "SignalHandler.h"

namespace ISXSignalHandler
{
void SignalHandler::HandleSignal(int signal)
{
	if (signal == SIGINT)
	{
		std::cerr << "\nProgram interrupted by user (SIGINT) with ^C. "
										 "Exiting...\n";
		std::exit(EXIT_SUCCESS);
	}
}

void SignalHandler::SetupSignalHandlers() { std::signal(SIGINT, HandleSignal); }
}