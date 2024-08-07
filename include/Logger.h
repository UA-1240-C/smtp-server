#pragma once

#ifdef _MSC_VER
#define __FUNCTION_SIGNATURE__ __FUNCSIG__
#else
#define __FUNCTION_SIGNATURE__ __PRETTY_FUNCTION__
#endif


#ifndef LOGGER_H
#define LOGGER_H

#pragma once
#include <cassert>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/config/user.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/common.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/exception_handler_feature.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/record_ordering.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/thread/thread.hpp>


namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace attrs = boost::log::attributes;

enum
{
	NONE_LVL = 0,
	PROD_AND_WARNING_AND_ERROR_LVL = 1,
	DEBUG_LVL = 2,
	TRACE_LVL = 3
};

enum LogLevel
{
	TRACE,
	DEBUG,
	PROD,
	WARNING,
	ERROR
};

//#define SIGN __FILE__ <<":(" << __LINE__ << ")" << __FUNCTION_SIGNATURE__

using text_sink = sinks::synchronous_sink<sinks::text_ostream_backend>;

namespace ISXLogger
{
	inline int log_level;
	inline std::mutex log_mutex;
	inline std::string log_file;
	inline bool flush;

	//// functions to work with parser
	std::string set_log_file(const std::string& parser_filename);
	[[nodiscard]] std::string get_log_file();

	bool set_flush(const int& parser_flush);
	[[nodiscard]] bool get_flush();

	int set_log_level(const int& parsing_log_level);

	[[nodiscard]] std::string get_log_level();

	src::severity_logger<LogLevel> InitLogging(src::severity_logger<LogLevel>& slg);

	void WarningLog(src::severity_logger<LogLevel>& slg, const std::string& message);
	void ErrorLog(src::severity_logger<LogLevel>& slg, const std::string& message);
	void DebugLog(src::severity_logger<LogLevel>& slg, const std::string& message);
	void TraceLog(src::severity_logger<LogLevel>& slg, const std::string& message);
};


#endif // LOGGER_H
