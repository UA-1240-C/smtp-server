#include "Logger.h"
// TODO: write Google tests
int main()
{
	Logger::InitLogging();
	Logger& logger = Logger::get_logger();
	logger.set_log_file("serverlog.txt");
	logger.set_log_level(TRACE);
	logger.set_flush(1);

	constexpr int number_of_threads = 18;

	DEBUG_LOGGER("debug message!");

	auto logAction = [](const int threadId)
	{
		for (int j = 0; j < 5; ++j)
		{
			TRACE_LOGGER("log from thread no." + std::to_string(threadId));
		}
	};

	std::vector<std::thread> threads;
	for (int i = 0; i < number_of_threads; ++i)
	{
		threads.emplace_back(logAction, i);
	}

	for (auto& thread : threads)
	{
		thread.join();
	}

	DEBUG_LOGGER("debug message!");
	Logger::StopLogging();
	return 0;
}
