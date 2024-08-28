#pragma once

#include <filesystem>
#include <fstream>
#include <ostream>
#include <string>
#include <thread>

#include <boost/core/null_deleter.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/common.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>

#include "LoggerTestUtilities.h"

namespace logging = boost::log;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace keywords = boost::log::keywords;

/**
 * @brief Macro for easier usage and better readability, enables short function signature
 */
#define ENABLE_FUNCTION_TRACING BOOST_LOG_FUNC()

inline constexpr uint8_t MAX_THREAD_COUNT = 5;

/**
 * @enum LogLevel
 * @brief Enum class for log levels.
 *
 * This enum consists of log levels that are used directly in the logger.
 */
enum LogLevel
{
	PROD, // Production log level: for e.g. Server start/stop/restart
	WARNING, // Warning log level: for e.g. Invalid input
	ERR, // Error log level: for e.g. Server crash
	DEBUG, // Debug log level: for e.g. Start and End functions, if configuration is successfully parsed
	TRACE // Trace log level: for e.g. Detailed information about the function
	// (input params, and return value (still to be added))
};

/**
 * @enum SeverityFilter
 * @brief Enum class for severity filter.
 *
 * This enum represents four possible severity levels in config.txt,
 * from which all the settings are being parsed.
 */
enum SeverityFilter
{
	NO_LOGS, // No logs, 0 in config.txt
	PROD_WARN_ERR_LOGS, // Production, Warning and Error logs, 1 in config.txt
	DEBUG_LOGS, // Debug logs, 2 in config.txt
	TRACE_LOGS // Trace logs, 3 in config.txt
};

/**
 * @struct Colors
 * @brief Struct for coloring console output.
 *
 * This struct consists of four colors that are used to color the console output,
 * the colors are being set depending on current severity level set. Colors are
 * represented as ANSI escape sequences.
 */
struct Colors
{
	static const std::string RED; // Red for Production/Warning/Error logs
	static const std::string BLUE; // Blue for Debug logs
	static const std::string CYAN; // Cyan for Trace logs
	static const std::string RESET; // Reset for clean overall look.
};

/**
 * Global severity logger instance for multithreaded logging.
 * This logger uses the specified LogLevel for filtering log messages.
 *
 * @note This instance is thread-safe due to `src::severity_logger_mt`.
 */
inline src::severity_logger_mt<LogLevel> g_slg;

/**
 * @class Logger
 * @brief Logger class for managing and writing log messages.
 *
 * This class handles all the logging operations, such as setting up the logger,
 * resetting it, working with different log levels, and writing log messages to the console.
 * Class implements Singleton pattern, so only one instance of the logger can be created.
 *
 */
class Logger
{
	Logger() = default;

	/**
	* @brief Destructor for the Logger class.
	*
	* Waits for any ongoing logs to end, then cleans the sink up and resets the logger.
	*/
	~Logger();

public:
	// Boost sink pointer for synchronous operations
	static boost::shared_ptr<sinks::synchronous_sink<sinks::text_ostream_backend>> s_sink_pointer;
	static uint8_t s_severity_filter; // Severity filter for the sink
	static std::string s_log_file; // Log file path, in development
	static bool s_flush; // Auto flushing for console output
	static std::mutex s_logging_mutex; // STL mutex for thread safety
	static ISXThreadPool::ThreadPool<> s_thread_pool; // Thread pool for multithreaded logging

	Logger(const Logger&) = delete;
	Logger& operator=(const Logger&) = delete;

	/**
	 * @brief Set sink for the logger.
	 *
	 * Function sets up the sink as shared pointer to the backend.
	 * Auto flushing is also being set here (if enabled).
	 */
	static boost::shared_ptr<sinks::synchronous_sink<sinks::text_ostream_backend>> set_sink();

	/**
	 * @brief Set global attributes for the logger.
	 *
	 * Function sets additional needed attributes to the logger:
	 *  - TimeStamp: current time
	 *	- Scope: current function scope
	 */
	static void set_attributes();

	/**
	 * @brief Set sink filter for the logger.
	 *
	 * Function sets the sink filter based on the severity filter set in the config.txt.
	 * Filter only passes those severity levels that are set in the config.txt and ignores the rest.
	 */
	static void set_sink_filter();

	/**
	 * @brief Set sink formatter for the logger.
	 *
	 * Sets the formatter for the sink as following:
	 * Thread ID - dd/mm/yyyy hh:mm:ss:mmm [LogLevel] – [Function/Method name] Log message.
	 */
	static void set_sink_formatter();

	/**
	 * @brief Setups the logger.
	 * @param logging_config Config::Logging instance that consists log level, log file name, log flush
	 */
	static void Setup(const Config::Logging& logging_config);

	/**
	 * @brief Resets the logger.
	 *
	 * Waits for all the ongoing logs to end, then removes all the sinks and resets the logger.
	 */
	static void Reset();

	/**
	 *
	 * @param message User-defined message to log
	 * @param log_level Log level of the message, one from LogLevel enum
	 */
	static void LogToConsole(const std::string& message, const LogLevel& log_level);

	/**
	 * @brief Logs the message with the DEBUG log level.
	 * @param message User-defined message to log
	 */
	static void LogDebug(const std::string& message);

	/**
	 * @brief Logs the message with the TRACE log level.
	 * @param message User-defined message to log
	 */
	static void LogTrace(const std::string& message);

	/**
	* @brief Logs the message with the PRODUCTION log level.
	* @param message User-defined message to log
	*/
	static void LogProd(const std::string& message);

	/**
	* @brief Logs the message with the WARNING log level.
	* @param message User-defined message to log
	*/
	static void LogWarning(const std::string& message);

	/**
	* @brief Logs the message with the ERROR log level.
	* @param message User-defined message to log
	*/
	static void LogError(const std::string& message);
};
