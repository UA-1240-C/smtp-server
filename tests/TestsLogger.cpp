#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Logger.h"

class MockThreadPool : public ISXThreadPool::ThreadPool<> {
public:
    MOCK_METHOD(void, EnqueueDetach, (std::function<void()>), (override));
};

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::s_thread_pool = std::make_shared<MockThreadPool>();
    }

    void TearDown() override {
        Logger::Reset();
    }
};

TEST_F(LoggerTest, SetupLoggerTest) {
    Config::Logging config{ "test.log", DEBUG_LOGS, true };
    EXPECT_NO_THROW(Logger::Setup(config));

    EXPECT_EQ(Logger::s_log_file, "test.log");
    EXPECT_EQ(Logger::s_severity_filter, DEBUG_LOGS);
    EXPECT_EQ(Logger::s_flush, true);
}

TEST_F(LoggerTest, LogDebugTest) {
    std::string message = "Debug message";
    EXPECT_CALL(*static_cast<MockThreadPool *>(Logger::s_thread_pool.get()), EnqueueDetach)
        .Times(1)
        .WillOnce(::testing::Invoke([&](std::function<void()> task) {
        task();  // Immediately execute the task for testing
            }));

    EXPECT_NO_THROW(Logger::LogDebug(message));
}

TEST_F(LoggerTest, LogTraceTest) {
    std::string message = "Trace message";
    EXPECT_CALL(*static_cast<MockThreadPool *>(Logger::s_thread_pool.get()), EnqueueDetach)
        .Times(1)
        .WillOnce(::testing::Invoke([&](std::function<void()> task) {
        task();  // Immediately execute the task for testing
            }));

    EXPECT_NO_THROW(Logger::LogTrace(message));
}

TEST_F(LoggerTest, LogProdTest) {
    std::string message = "Prod message";
    EXPECT_CALL(*static_cast<MockThreadPool *>(Logger::s_thread_pool.get()), EnqueueDetach)
        .Times(1)
        .WillOnce(::testing::Invoke([&](std::function<void()> task) {
        task();  // Immediately execute the task for testing
            }));

    EXPECT_NO_THROW(Logger::LogProd(message));
}

TEST_F(LoggerTest, LogWarningTest) {
    std::string message = "Warning message";
    EXPECT_CALL(*static_cast<MockThreadPool *>(Logger::s_thread_pool.get()), EnqueueDetach)
        .Times(1)
        .WillOnce(::testing::Invoke([&](std::function<void()> task) {
        task();  // Immediately execute the task for testing
            }));

    EXPECT_NO_THROW(Logger::LogWarning(message));
}

TEST_F(LoggerTest, LogErrorTest) {
    std::string message = "Error message";
    EXPECT_CALL(*static_cast<MockThreadPool *>(Logger::s_thread_pool.get()), EnqueueDetach)
        .Times(1)
        .WillOnce(::testing::Invoke([&](std::function<void()> task) {
        task();  // Immediately execute the task for testing
            }));

    EXPECT_NO_THROW(Logger::LogError(message));
}

TEST_F(LoggerTest, ResetTest) {
    EXPECT_NO_THROW(Logger::Reset());

    // Check that sinks are removed
    EXPECT_EQ(Logger::s_sink_pointer, nullptr);

    // Check that thread pool is waiting for tasks
    // This is indirectly tested by the thread pool mock's destructor being called.
}

TEST_F(LoggerTest, LogToConsoleTest) {
    std::stringstream buffer;
    std::streambuf *old = std::clog.rdbuf(buffer.rdbuf());  // Redirect std::clog

    std::string message = "Test message";
    EXPECT_NO_THROW(Logger::LogToConsole(message, DEBUG));

    std::clog.rdbuf(old);  // Restore std::clog
    EXPECT_NE(buffer.str().find("Test message"), std::string::npos);
}