#pragma once

#ifdef _MSC_VER
#define __FUNCTION_SIGNATURE__ __FUNCSIG__
#else
#define __FUNCTION_SIGNATURE__ __PRETTY_FUNCTION__
#endif


#ifndef LOGGER_H
#define LOGGER_H

#pragma once
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/attributes/counter.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/attributes/timer.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/expressions/formatters/if.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/exception_handler_feature.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/file.hpp>


namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace attrs = boost::log::attributes;


// TODO: rewrite macros for boost
#define LOG(logger, message) ((logger).Log((logger).get_log_level(), std::nullopt, message, __FILE__, __LINE__))
#define LOG_TRACE(logger, message) ((logger).Log(LogLevel::TRACE, __FUNCTION_SIGNATURE__, message, __FILE__, __LINE__))
// TODO: set auto start/end(+restart for server) functions logging (by scope?)

BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(scope, "Scope", attrs::named_scope)
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", logging::trivial::severity_level)

/*!
 * Macro for function scope markup. The scope name is constructed with help of compiler and contains the current function signature.
 * The scope name is pushed to the end of the current thread scope list.
 */
#define BOOST_LOG_FUNCTION()\
    BOOST_LOG_NAMED_SCOPE_INTERNAL(BOOST_LOG_UNIQUE_IDENTIFIER_NAME(_boost_log_named_scope_sentry_), BOOST_CURRENT_FUNCTION, __FILE__, __LINE__, ::boost::log::attributes::named_scope_entry::function)

#define TRACE_LOGGER() \
    BOOST_LOG_TRIVIAL(TRACE) << BOOST_LOG_FUNCTION() << ": function where logger is called"

 // /*! if previous one doesnt work:
 //  * Macro for function scope markup. The scope name is constructed with help of compiler and contains the current function name. It may be shorter than what \c BOOST_LOG_FUNCTION macro produces.
 //  * The scope name is pushed to the end of the current thread scope list.
 //  */
 //#if defined(_MSC_VER) || defined(__GNUC__)
 //#define BOOST_LOG_FUNC() BOOST_LOG_NAMED_SCOPE(__FUNCTION__)
 //#else
 //#define BOOST_LOG_FUNC() BOOST_LOG_FUNCTION()
 //	#endif

 /*
  * <logging>
		 <!--path to log file-->
		 <filename>"serverlog.txt"</filename>
		 <!--supports only 3 levels of logging: 0 - No logs, 1 - Production logs/Warning/Erros, 2 - Debug logs, 3 - Trace logs-->
		 <LogLevel>"2"</LogLevel>
		 <!--parameter for flushing output to log file (1 - enable / 0 - disable). Warning this parameter will slow dowm output!-->
		 <flush>"0"</flush>
	 </logging>

  */

	using sink_ostream = sinks::asynchronous_sink<sinks::text_ostream_backend>;

enum LogLevel
{
	TRACE = logging::trivial::trace,
	DEBUG = logging::trivial::debug,
	PROD = logging::trivial::info,
	WARNING = logging::trivial::warning,
	ERROR = logging::trivial::error
};


class Logger
{
	// functions to work with parser
	std::string set_log_file(const std::string &filename);
	std::string get_log_file() const;
	LogLevel set_log_level(const int &log_level);
	LogLevel get_log_level() const;
	bool set_flush(const int &flush);
	bool get_flush() const;

	Logger() = default;
	~Logger() = default;
	Logger(const Logger &) = delete;
	Logger &operator=(const Logger &) = delete;

	std::string m_log_file;
	int m_log_level;
	bool m_flush;
	LogLevel m_current_log_level;

public:
	static std::mutex s_log_mutex;

	static Logger &get_logger();

	static boost::shared_ptr<sink_ostream> init_logging();
	static void stop_logging();
};


#endif // LOGGER_H
