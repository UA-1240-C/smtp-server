#include <boost/test/unit_test.hpp>
#include "Logger.h"


BOOST_AUTO_TEST_SUITE(TestLogger)

BOOST_AUTO_TEST_CASE(TestLoggerSetup)
{
	Config::Logging config;
	config.filename = "serverlog.txt";
	config.log_level = DEBUG_LOGS;
	config.flush = true;

	Logger::Setup(config);

	BOOST_CHECK_EQUAL(Logger::get_log_file(), "serverlog.txt");
	BOOST_CHECK_EQUAL(Logger::get_severity_filter(), DEBUG_LOGS);
	BOOST_CHECK_EQUAL(Logger::get_flush(), 1);
	BOOST_REQUIRE(Logger::get_sink_pointer() != nullptr);

	Logger::Reset();
}

BOOST_AUTO_TEST_CASE(TestLogToConsole)
{
	// Redirect std::cout to capture console output
	std::ostringstream output_stream;
	std::streambuf *old_cout_buf = std::cout.rdbuf(output_stream.rdbuf());

	Config::Logging config;
	config.filename = "serverlog.txt";
	config.log_level = DEBUG_LOGS;
	config.flush = true;

	// Setup Logger with DEBUG_LOGS level
	Logger::Setup(config);
	Logger::LogToConsole("Test Debug Message", DEBUG);
	Logger::Reset();
	config.log_level = PROD_WARN_ERR_LOGS;
	Logger::Setup(config);
	Logger::LogToConsole("Test Error Message", ERR);

	// Launch multiple threads to log messages concurrently
	std::vector<std::thread> log_threads;
	for (int i = 0; i < 10; ++i)
	{
		log_threads.emplace_back([i]()
			{
				Logger::LogToConsole("Test Debug Message " + std::to_string(i), DEBUG);
				std::cout.flush(); // Force flush after each log
			});
	}

	// Wait for all threads to complete logging
	for (auto &thread : log_threads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}

	// Force flush to ensure all output is written
	std::cout.flush();

	// Capture and validate the output
	std::string output = output_stream.str();

	// Restore std::cout
	std::cout.rdbuf(old_cout_buf);

	for (int i = 0; i < 10; ++i)
	{
		std::string expected_message = "Test Debug Message " + std::to_string(i);
		BOOST_REQUIRE_MESSAGE(output.find(expected_message) != std::string::npos,
			"Message not found: " + expected_message);
	}

	Logger::Reset();
}

BOOST_AUTO_TEST_CASE(TestLoggerAsync)
{
	// Redirect std::cout to capture console output
	std::ostringstream output_stream;
	std::streambuf *old_cout_buf = std::cout.rdbuf(output_stream.rdbuf());

	Config::Logging config;
	config.filename = "serverlog.txt";
	config.log_level = TRACE_LOGS;
	config.flush = true;

	Logger::Setup(config);
	Logger::LogDebug("Debug message");
	Logger::LogTrace("Trace message");
	Logger::LogProd("Prod message");

	std::string output = output_stream.str();

	// Restore std::cout
	std::cout.rdbuf(old_cout_buf);

	BOOST_REQUIRE(output.find("Debug message") == std::string::npos);
	BOOST_REQUIRE(output.find("Trace message") != std::string::npos);
	BOOST_REQUIRE(output.find("Prod message") == std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestLoggerReset)
{
	Config::Logging config;
	config.filename = "test.log";
	config.log_level = DEBUG_LOGS;
	config.flush = true;
	Logger::Setup(config);

	Logger::Reset();

	BOOST_REQUIRE(Logger::get_sink_pointer() == nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
