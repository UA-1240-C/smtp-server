#pragma once

#include <fstream>
#include <ostream>
#include <string>

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
#include <boost/smart_ptr/shared_ptr.hpp>

namespace logging = boost::log;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace keywords = boost::log::keywords;

#define ENABLE_FUNCTION_TRACING BOOST_LOG_FUNC()

enum LogLevel
{
	PROD,
	WARNING,
	ERROR,
	DEBUG,
	TRACE
};

enum SeverityFilter
{
	NO_LOGS,
	PROD_WARN_ERR_LOGS,
	DEBUG_LOGS,
	TRACE_LOGS
};

inline src::severity_logger_mt<LogLevel> g_slg;
inline src::logger_mt g_lg;


class Logger
{
	static boost::shared_ptr<sinks::synchronous_sink<sinks::text_ostream_backend>> s_sink_pointer;
	static int s_severity_filter;

	Logger() = default;
	~Logger();

public:
	Logger(const Logger&) = delete;
	Logger& operator=(const Logger&) = delete;

	static void set_attributes();
	static void set_sink_filter();

	static boost::shared_ptr<sinks::synchronous_sink<sinks::text_ostream_backend>> set_sink();

	static void set_sink_formatter(
		const boost::shared_ptr<sinks::synchronous_sink<sinks::text_ostream_backend>>& sink_point
	);

	static void Setup(const int& severity);
	static void Reset();

	static void LogDebug(const std::string& message);
	static void LogTrace(const std::string& message);
	static void LogProd(const std::string& message);
	static void LogWarning(const std::string& message);
	static void LogError(const std::string& message);
};
