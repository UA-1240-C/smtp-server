#include "Logger.h"

// minimal example
int foo(src::logger_mt& lg)
{
	BOOST_LOG_FUNC()
	ISXLogger::Trace("Tracing callable function...");
	return 10;
}

int main()
{
	ISXLogger::Setup(TRACE_LOGS);
	ISXLogger::Trace("Tracing main function...");
	ISXLogger::Debug("Debugging message; will not pass");
	ISXLogger::Trace("Tracing main function... again");
	ISXLogger::Reset();
	return 0;
}
