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
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/exception_handler_feature.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/config/user.hpp>
#include <boost/thread/thread.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/utility/record_ordering.hpp>


namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace attrs = boost::log::attributes;


enum LogLevel
{
	TRACE = logging::trivial::trace,
	DEBUG = logging::trivial::debug,
	PROD = logging::trivial::info,
	WARNING = logging::trivial::warning,
	ERROR = logging::trivial::error
};


BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(scope, "Scope", attrs::named_scope)
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", LogLevel)

BOOST_LOG_GLOBAL_LOGGER(g_logger, src::logger_mt)

#define TRACE_LOGGER(message) (Logger::get_logger().WriteLog(TRACE, __FUNCTION_SIGNATURE__, __FILE__, __LINE__, message))

// TODO: set auto functions logging (by scope?)
#define DEBUG_LOGGER(message) (Logger::get_logger().WriteLog(DEBUG, __func__, __FILE__, __LINE__, message))


using sink_ostream = sinks::asynchronous_sink<sinks::text_ostream_backend>;
//using sink_fstream = sinks::asynchronous_sink<sinks::text_file_backend>;

class Logger
{
	Logger() = default;
	~Logger() = default;

	std::string m_log_file;
	LogLevel m_log_level;
	bool m_flush;
	src::severity_logger_mt<LogLevel> m_logger;
	boost::shared_ptr<sink_ostream> m_file_sink;

public:
	Logger(const Logger&) = delete;
	Logger& operator=(const Logger&) = delete;

	static std::mutex s_log_mutex;

	// functions to work with parser
	std::string set_log_file(const std::string& filename);
	[[nodiscard]] std::string get_log_file() const;
	LogLevel set_log_level(const int& log_level);
	[[nodiscard]] LogLevel get_log_level() const;
	bool set_flush(const int& flush);
	[[nodiscard]] bool get_flush() const;
	src::severity_logger_mt<LogLevel> get_m_logger() const;
	boost::shared_ptr<sink_ostream> set_file_sink(const boost::shared_ptr<sink_ostream>& file_sink);
	[[nodiscard]] boost::shared_ptr<sink_ostream> get_file_sink() const;

	static Logger& get_logger();

	static boost::shared_ptr<sink_ostream> InitLogging();
	static void WriteLog(LogLevel, const std::string& function_name, const std::string& file, const int& line,
						const std::string& message);
	static void StopLogging();
};


#endif // LOGGER_H
