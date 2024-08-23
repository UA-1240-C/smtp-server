#include <boost/test/unit_test.hpp>
#include "Logger.h"

#ifndef BOOST_TEST_MODULE
#error "BOOST_TEST_MODULE is not defined. Make sure to define it in your CMake configuration."
#endif

BOOST_AUTO_TEST_SUITE(TestLogger)

	BOOST_AUTO_TEST_CASE(TestLoggerSetup)
	{
		Config::Logging config;
		config.filename = "serverlog.txt";
		config.log_level = DEBUG_LOGS;
		config.flush = true;

		Logger::Setup(config);

		BOOST_CHECK_EQUAL(Logger::s_log_file, "serverlog.txt");
		BOOST_CHECK_EQUAL(Logger::s_severity_filter, DEBUG_LOGS);
		BOOST_CHECK_EQUAL(Logger::s_flush, 1);
		BOOST_REQUIRE(Logger::s_sink_pointer != nullptr);

		Logger::Reset();
	}

	BOOST_AUTO_TEST_CASE(TestLogToConsole)
	{
		// Redirect std::cout to capture console output
		std::ostringstream output_stream;
		std::streambuf* old_cout_buf = std::cout.rdbuf(output_stream.rdbuf());

		Config::Logging config;
		config.filename = "serverlog.txt";
		config.log_level = DEBUG_LOGS;
		config.flush = true;

		Logger::Setup(config);
		Logger::LogToConsole("Test Debug Message", DEBUG);
		Logger::Reset();
		config.log_level = PROD_WARN_ERR_LOGS;
		Logger::Setup(config);
		Logger::LogToConsole("Test Error Message", ERR);

		// Assuming that Log functions are asynchronous, wait for the log to flush
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		std::string output = output_stream.str();

		// Restore std::cout
		std::cout.rdbuf(old_cout_buf);

		BOOST_REQUIRE(output.find("Test Debug Message") != std::string::npos);
		BOOST_REQUIRE(output.find(Colors::BLUE) != std::string::npos);
		BOOST_REQUIRE(output.find("Test Error Message") != std::string::npos);
		BOOST_REQUIRE(output.find(Colors::RED) != std::string::npos);

		Logger::Reset();
	}

	BOOST_AUTO_TEST_CASE(TestLoggerAsync)
	{
		Config::Logging config;
		config.filename = "test.log";
		config.log_level = TRACE_LOGS;
		config.flush = true;

		Logger::Setup(config);
		Logger::LogDebug("Debug message");
		Logger::LogTrace("Trace message");
		Logger::LogProd("Prod message");

		// Assuming that Log functions are asynchronous, wait for the log to flush
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		// Open the log file and check for content
		std::ifstream log_file("test.log");
		std::stringstream log_contents;
		log_contents << log_file.rdbuf();
		log_file.close();

		std::string output = log_contents.str();

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

		BOOST_REQUIRE(Logger::s_sink_pointer == nullptr);
	}

BOOST_AUTO_TEST_SUITE_END()
