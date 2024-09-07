#include <iostream>
#include <thread>
#include "Logger.h"

// Function for each thread to log messages
void LogMessages(int thread_id)
{
	std::string msg_base = "Thread " + std::to_string(thread_id) + " - ";

	Logger::LogDebug(msg_base + "Debug message");
	Logger::LogTrace(msg_base + "Trace message");
	Logger::LogProd(msg_base + "Production message");
	Logger::LogWarning(msg_base + "Warning message");
	Logger::LogError(msg_base + "Error message");
}

int main()
{
	// Configure the logger (this would normally come from a config file)
	Config::Logging logging_config;
	logging_config.filename = "server.log";
	logging_config.log_level = TRACE_LOGS;
	logging_config.flush = true;

	// Initialize the logger with the configuration
	Logger::Setup(logging_config);

	// Number of threads to simulate
	constexpr int num_threads = 5;
	std::thread threads[num_threads];

	// Launch threads
	for (int i = 0; i < num_threads; ++i)
	{
		threads[i] = std::thread(LogMessages, i + 1);
	}

	// Join threads
	for (int i = 0; i < num_threads; ++i)
	{
		threads[i].join();
	}

	// Reset the logger (clean up resources)
	Logger::Reset();

	std::cout << "Logging complete. Check the console for output." << std::endl;

	return 0;
}
