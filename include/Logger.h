#pragma once
#ifndef LOGGER_H
#define LOGGER_H

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

inline src::severity_logger_mt<LogLevel> slg;
inline src::logger_mt lg;

namespace ISXLogger
{
	inline boost::shared_ptr<sinks::synchronous_sink<sinks::text_ostream_backend>> sink_pointer;
	inline int severity_filter;

	void SetAttributes();
	void SetSinkFilter();

	boost::shared_ptr<sinks::synchronous_sink<sinks::text_ostream_backend>> SetSink();

	void SetSinkFormatter(const boost::shared_ptr<sinks::synchronous_sink<sinks::text_ostream_backend>>& sink_point);

	void Setup(const int& severity);
	void Reset();

	void Debug(const std::string& message);
	void Trace(const std::string& message);
	void Prod(const std::string& message);
	void Warning(const std::string& message);
	void Error(const std::string& message);
}

#endif
