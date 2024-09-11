#include <gtest/gtest.h>
#include <fstream>
#include <thread>
#include <vector>
#include <string>
#include <cstdio>  // For std::remove
#include "Logger.h"

// Test fixture for Logger tests
class LoggerTest : public testing::Test
{
protected:
	void SetUp() override
	{
		// Reset the logger before each test to ensure a clean slate
		Logger::Reset();
	}

	void TearDown() override
	{
		// Clean up log files after tests
		std::remove("test.log");
		std::remove("file_test.log");
		std::remove("level_test.log");
		std::remove("multi_threaded.log");
		std::remove("high_concurrency.log");
		std::remove("special_chars.log");
	}
};

// Test to verify logger setup with a configuration
TEST_F(LoggerTest, SetupConfiguresLogger)
{
	Config::Logging config;
	config.filename = "test.log";
	config.log_level = TRACE_LOGS;
	config.flush = true;
	Logger::Setup(config);

	// Verify that the log file is created and can be opened
	std::ifstream file("test.log");
	ASSERT_TRUE(file.is_open());
}

// Test that log messages are written to the specified file
TEST_F(LoggerTest, LogFileWriting)
{
	Config::Logging config;
	config.filename = "file_test.log";
	config.log_level = DEBUG_LOGS;
	config.flush = true;
	Logger::Setup(config);

	Logger::LogDebug("Test file write", std::source_location::current());

	// Verify that the log file contains the expected message
	std::ifstream log_file("file_test.log");
	std::string file_content((std::istreambuf_iterator<char>(log_file)),
							std::istreambuf_iterator<char>());
	EXPECT_NE(file_content.find("Test file write"), std::string::npos);
}

// Test concurrent logging with multiple threads
TEST_F(LoggerTest, ConcurrentLogging)
{
	Config::Logging config;
	config.filename = "file_test.log";
	config.log_level = DEBUG_LOGS;
	config.flush = true;
	Logger::Setup(config);

	auto log_task = [](const std::string& msg)
	{
		Logger::LogDebug(msg);
	};

	std::thread t1(log_task, "Concurrent message 1");
	std::thread t2(log_task, "Concurrent message 2");

	t1.join();
	t2.join();

	// Check if both messages were written to the log file
	std::ifstream log_file("file_test.log");
	std::string file_content((std::istreambuf_iterator<char>(log_file)),
							std::istreambuf_iterator<char>());
	EXPECT_NE(file_content.find("Concurrent message 1"), std::string::npos);
	EXPECT_NE(file_content.find("Concurrent message 2"), std::string::npos);
}

// Test logger behavior with an empty configuration
TEST_F(LoggerTest, EmptyConfiguration)
{
	Config::Logging config;
	config.filename = ""; // No file specified
	config.log_level = NO_LOGS; // No logs should be generated
	config.flush = false;
	Logger::Setup(config);

	// Attempt to log a message
	Logger::LogDebug("Should not appear");

	// Verify that no log file is created
	std::ifstream file("test.log");
	EXPECT_FALSE(file.is_open());

	Logger::Reset();
}

// Test that log level filtering works correctly
TEST_F(LoggerTest, LogLevelFiltering)
{
	Config::Logging config;
	config.filename = "level_test.log";
	config.log_level = PROD_WARN_ERR_LOGS;
	config.flush = true;
	Logger::Setup(config);

	// Log messages at different levels
	Logger::LogDebug("Debug message");
	Logger::LogWarning("Warning message");
	Logger::LogError("Error message");

	// Verify that only warning and error messages are logged
	std::ifstream file("level_test.log");
	std::string content((std::istreambuf_iterator<char>(file)),
						std::istreambuf_iterator<char>());

	EXPECT_EQ(content.find("Debug message"), std::string::npos); // Should not be logged
	EXPECT_NE(content.find("Warning message"), std::string::npos);
	EXPECT_NE(content.find("Error message"), std::string::npos);

	Logger::Reset();
}

// Test that exceptions are properly handled by the logger
TEST_F(LoggerTest, ExceptionHandling)
{
	Config::Logging config;
	config.filename = "/invalid/path/to/logfile.log"; // Invalid path
	config.log_level = DEBUG_LOGS;
	config.flush = true;

	try
	{
		Logger::Setup(config);
		Logger::LogDebug("This should handle exceptions");
	}
	catch (const std::exception& e)
	{
		// Ensure that the exception is handled and logged appropriately
		std::cerr << e.what() << '\n';
	}

	// Verify no crash occurs and logger handles the situation gracefully
	Logger::Reset();
}

// Test high-concurrency logging to ensure thread safety
TEST_F(LoggerTest, HighConcurrencyLogging)
{
	Config::Logging config;
	config.filename = "high_concurrency.log";
	config.log_level = DEBUG_LOGS;
	config.flush = true;
	Logger::Setup(config);

	constexpr int thread_count = 100;
	std::vector<std::thread> threads;

	for (int i = 0; i < thread_count; ++i)
	{
		threads.emplace_back([i]()
		{
			Logger::LogDebug("Message from thread " + std::to_string(i));
		});
	}

	for (auto& thread : threads)
	{
		thread.join();
	}

	// Verify that all messages are logged
	std::ifstream file("high_concurrency.log");
	std::string content((std::istreambuf_iterator<char>(file)),
						std::istreambuf_iterator<char>());

	for (int i = 0; i < thread_count; ++i)
	{
		EXPECT_NE(content.find("Message from thread " + std::to_string(i)), std::string::npos);
	}

	Logger::Reset();
}

// Test logger behavior with special characters in the log message
TEST_F(LoggerTest, LogSpecialCharacters)
{
	Config::Logging config;
	config.filename = "special_chars.log";
	config.log_level = DEBUG_LOGS;
	config.flush = true;
	Logger::Setup(config);

	std::string special_message = "Special characters: !@#$%^&*()_+~`";
	Logger::LogDebug(special_message);

	// Verify that the special characters are logged correctly
	std::ifstream file("special_chars.log");
	std::string content((std::istreambuf_iterator<char>(file)),
						std::istreambuf_iterator<char>());
	EXPECT_NE(content.find(special_message), std::string::npos);

	Logger::Reset();
}

// Test logger behavior when the log file cannot be opened
TEST_F(LoggerTest, LogFileCannotBeOpened)
{
	Config::Logging config;
	config.filename = "/root/inaccessible_log.log"; // Inaccessible file path
	config.log_level = DEBUG_LOGS;
	config.flush = true;

	try
	{
		Logger::Setup(config);
		Logger::LogDebug("This log should fail to open the file.");
	}
	catch (const std::exception& e)
	{
		// Ensure that an appropriate exception is caught
		EXPECT_STREQ(e.what(), "Unable to open log file: /root/inaccessible_log.log");
	}

	// Verify that no log file is created
	std::ifstream file("/root/inaccessible_log.log");
	EXPECT_FALSE(file.is_open());

	Logger::Reset();
}

// Test that the sink is set correctly based on the configuration
TEST_F(LoggerTest, SinkSetup)
{
	Config::Logging config;
	config.filename = "test.log";
	config.log_level = TRACE_LOGS;
	config.flush = true;
	Logger::Setup(config);

	// Log a message
	Logger::LogDebug("Test sink setup");

	// Verify the log file was created and contains the message
	std::ifstream log_file("test.log");
	std::string file_content((std::istreambuf_iterator<char>(log_file)),
							std::istreambuf_iterator<char>());
	EXPECT_NE(file_content.find("Test sink setup"), std::string::npos);
}

// Test that the thread-local logger instance works correctly
TEST_F(LoggerTest, ThreadLocalLogger)
{
	Config::Logging config;
	config.filename = "thread_local.log";
	config.log_level = DEBUG_LOGS;
	config.flush = true;
	Logger::Setup(config);

	auto log_task = []()
	{
		Logger::LogDebug("Message from thread-local logger");
	};

	std::thread t1(log_task);
	t1.join();

	// Verify the message was logged by the thread-local logger
	std::ifstream log_file("thread_local.log");
	std::string file_content((std::istreambuf_iterator<char>(log_file)),
							std::istreambuf_iterator<char>());
	EXPECT_NE(file_content.find("Message from thread-local logger"), std::string::npos);
}

// Test logging output when no sinks are available
TEST_F(LoggerTest, NoSinksAvailable)
{
	Logger::Reset(); // Remove all sinks manually
	try
	{
		Logger::LogDebug("This should not be logged");
	}
	catch (const std::exception& e)
	{
		// Since no sink is available, ensure that an appropriate exception or behavior is observed
		EXPECT_TRUE(false) << "Logger should handle missing sinks gracefully.";
	}

	// Verify that no log file is created
	std::ifstream file("test.log");
	EXPECT_FALSE(file.is_open());
}

// Test that the logger flushes output correctly based on configuration
TEST_F(LoggerTest, FlushOutput)
{
	Config::Logging config;
	config.filename = "flush_test.log";
	config.log_level = DEBUG_LOGS;
	config.flush = true;
	Logger::Setup(config);

	Logger::LogDebug("Flush test");

	// Verify that the log file contains the message, indicating the flush was successful
	std::ifstream log_file("flush_test.log");
	std::string file_content((std::istreambuf_iterator<char>(log_file)),
							std::istreambuf_iterator<char>());
	EXPECT_NE(file_content.find("Flush test"), std::string::npos);
}

// Test logging with different colors based on log severity
TEST_F(LoggerTest, ColorLogging)
{
	Config::Logging config;
	config.filename = "color_test.log";
	config.log_level = TRACE_LOGS;
	config.flush = true;
	Logger::Setup(config);

	Logger::LogDebug("Debug message");
	Logger::LogWarning("Warning message");
	Logger::LogError("Error message");

	// Verify that the log file contains the colored messages (escaped sequences)
	std::ifstream log_file("color_test.log");
	std::string file_content((std::istreambuf_iterator<char>(log_file)),
							std::istreambuf_iterator<char>());

	EXPECT_NE(file_content.find(Colors::BLUE + "Debug message" + Colors::RESET), std::string::npos);
	EXPECT_NE(file_content.find(Colors::RED + "Warning message" + Colors::RESET), std::string::npos);
	EXPECT_NE(file_content.find(Colors::RED + "Error message" + Colors::RESET), std::string::npos);
}

TEST_F(LoggerTest, LogToServerlog)
{
	Config::Logging config;
	config.filename = "serverlog.log";
	config.log_level = DEBUG_LOGS;
	config.flush = true;
	Logger::Setup(config);

	Logger::LogDebug("Test serverlog write");

	// Verify that the log file contains the expected message
	std::ifstream log_file("serverlog.log");
	std::string file_content((std::istreambuf_iterator<char>(log_file)),
							std::istreambuf_iterator<char>());
	EXPECT_NE(file_content.find("Test serverlog write"), std::string::npos);
}

// Main function to run all the tests
int main(int argc, char** argv)
{
	{
		Config::Logging c;
		Logger::Setup(c);
		Logger::LogDebug("Test serverlog write");
	}
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
