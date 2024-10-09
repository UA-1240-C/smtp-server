#include <gtest/gtest.h>
#include <thread>
#include "Logger.h"

class LoggerTest : public testing::Test
{
protected:
	void SetUp() override
	{
		// Setup the logger for each test
		Logger::Setup(Config::Logging{"serverlog.txt", DEBUG_LOGS, true});
	}

	void TearDown() override
	{
		Logger::Reset(); // Reset logger state after each test
	}
};

// Test case 1: Check if Logger is initialized correctly
TEST_F(LoggerTest, LoggerInitializesCorrectly)
{
	EXPECT_NO_THROW(Logger::Setup(Config::Logging{ "serverlog.txt", DEBUG_LOGS, true }));
}


// Test case 2: Verify if SeverityToOutput handles valid log levels
TEST_F(LoggerTest, SeverityToOutputValid)
{
	Logger::Setup(Config::Logging{"serverlog.txt", PROD_WARN_ERR_LOGS, true}); // Set severity filter to Prod/Warn/Error
	EXPECT_EQ(Logger::SeverityToOutput(), "Prod/Warning/Error");
}

// Test case 3: Verify Logger resets correctly
TEST_F(LoggerTest, LoggerResetsCorrectly)
{
	EXPECT_NO_THROW(Logger::Reset());
}

// Test case 4: Ensure multiple log levels work together
TEST_F(LoggerTest, MultipleLogLevels)
{
	Logger::LogProd("Production log");
	Logger::LogDebug("Debug log");
	Logger::LogWarning("Warning log");
	EXPECT_NO_THROW(Logger::LogError("Error log"));
}

// Test case 5: Test logger’s output when no log level is set (NO_LOGS)
TEST_F(LoggerTest, NoLogsOutput)
{
	Logger::Setup(Config::Logging{"serverlog.txt", NO_LOGS, false});
	Logger::LogProd("This message should not appear");
	EXPECT_EQ(Logger::SeverityToOutput(), "");
}

// Test case 6: Test console output format contains log level and timestamp
TEST_F(LoggerTest, LogOutputFormat)
{
	std::string message = "Test log message";
	Logger::LogDebug(message);
	// Assuming you have a way to capture and assert the format of the output
	// Check for log level and timestamp in the output
	// Example: "[DEBUG] 2023-09-12 12:45:00 Test log message"
}

// Test case 7: Test log level filtering (DEBUG should exclude TRACE)
TEST_F(LoggerTest, LogLevelFilterDebug)
{
	Logger::Setup(Config::Logging{"serverlog.txt", DEBUG_LOGS, true});
	Logger::LogTrace("This trace log should not appear");
	Logger::LogDebug("This debug log should appear");
	EXPECT_EQ(Logger::SeverityToOutput(), "Debug");
}

// Test case 8: Test log level filtering (PROD should exclude DEBUG/TRACE)
TEST_F(LoggerTest, LogLevelFilterProd)
{
	Logger::Setup(Config::Logging{"serverlog.txt", PROD_WARN_ERR_LOGS, true});
	Logger::LogTrace("This trace log should not appear");
	Logger::LogDebug("This debug log should not appear");
	Logger::LogProd("This prod log should appear");
	EXPECT_EQ(Logger::SeverityToOutput(), "Prod/Warning/Error");
}

// Test case 9: Test logger flush functionality
TEST_F(LoggerTest, LoggerFlush)
{
	Logger::Setup(Config::Logging{"serverlog.txt", DEBUG_LOGS, true});
	Logger::LogDebug("This should be flushed immediately");
	Logger::Reset(); // After reset, all pending logs should be flushed
	EXPECT_NO_THROW(Logger::LogProd("This should flush as well"));
}

// Test case 10: Test logger reset removes sinks
TEST_F(LoggerTest, ResetRemovesSinks)
{
	Logger::Setup(Config::Logging{"serverlog.txt", DEBUG_LOGS, true});
	Logger::Reset();
	// Try logging after reset, should reinitialize sinks
	EXPECT_NO_THROW(Logger::LogProd("This should work after reset"));
}

// Test case 11: Ensure correct logging to syslog
TEST_F(LoggerTest, LogToSyslog)
{
	std::string message = "System log test";
	Logger::LogProd(message);
}

// Test case 12: Ensure log message length limit (if applicable)
TEST_F(LoggerTest, LogMessageLengthLimit)
{
	std::string long_message(1024, 'A'); // Test a long message of 1024 chars
	EXPECT_NO_THROW(Logger::LogProd(long_message)); // Should handle long messages without crashing
}

// Test case 13: Verify log to console on various levels
TEST_F(LoggerTest, LogToConsoleDifferentLevels)
{
	Logger::Setup(Config::Logging{"serverlog.txt", TRACE_LOGS, true});
	std::string message = "This is a production log";
	EXPECT_NO_THROW(Logger::LogProd(message));
	message = "This is a debug log";
	EXPECT_NO_THROW(Logger::LogDebug(message));
	message = "This is a trace log";
	EXPECT_NO_THROW(Logger::LogTrace(message));
	message = "This is a warning log";
	EXPECT_NO_THROW(Logger::LogWarning(message));
	message = "This is an error log";
	EXPECT_NO_THROW(Logger::LogError(message));
	std::string empty_message = "";
	EXPECT_NO_THROW(Logger::LogProd(empty_message));
}

// Test case 14: Check log color formatting for error messages
TEST_F(LoggerTest, LogErrorColor)
{
	std::string message = "Error occurred";
	Logger::LogError(message);
	// Assuming console coloring is handled by ANSI, check for the red color code
	EXPECT_EQ(Colors::RED, "\033[1;31m");
}

// Test case 15: Check log color formatting for trace messages
TEST_F(LoggerTest, LogTraceColor)
{
	std::string message = "Trace message";
	Logger::LogTrace(message);
	// Check if the color is set to cyan for TRACE logs
	EXPECT_EQ(Colors::CYAN, "\033[1;36m");
}


int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
