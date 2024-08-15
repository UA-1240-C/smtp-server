#include "Logger.h"

// minimal example
int foo()
{
	ENABLE_FUNCTION_TRACING
	Logger::LogTrace("Tracing callable function...");
	Logger::LogDebug("Debugging!");
	return 10;
}

int main()
{
	ENABLE_FUNCTION_TRACING
	Logger::Setup(TRACE_LOGS);
	Logger::LogTrace("Tracing main function...");
	Logger::LogDebug("Debugging message; will not pass");
	foo();
	Logger::Reset();

	Logger::Setup(DEBUG_LOGS);
	Logger::LogTrace("Trying to trace... without success");
	foo();
	Logger::LogDebug("Debugging message from main!");
	Logger::Reset();
	return 0;
}
