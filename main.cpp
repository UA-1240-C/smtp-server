#include "Logger.h"

// minimal example
int foo(src::severity_logger<LogLevel>& lg)
{
	BOOST_LOG_FUNCTION()
	BOOST_LOG(lg) << "foo is being called";
	ISXLogger::TraceLog(lg, "tracing...");
	ISXLogger::DebugLog(lg, "debug log");
	return 10;
}

int main(int argc, char* argv[])
{
	BOOST_LOG_FUNCTION()
	src::severity_logger<LogLevel> slg;
	ISXLogger::set_log_level(3);
	slg = ISXLogger::InitLogging(slg);
	ISXLogger::set_log_file("serverlog.txt");
	ISXLogger::ErrorLog(slg, "error message");
	foo(slg);
	BOOST_LOG(slg) << foo(slg);

	return 0;
}
