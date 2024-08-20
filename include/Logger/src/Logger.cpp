#include "Logger.h"

boost::shared_ptr<sinks::synchronous_sink<sinks::text_ostream_backend>> Logger::s_sink_pointer;
int Logger::s_severity_filter;

void Logger::set_attributes()
{
	const attrs::local_clock time_stamp;
	logging::core::get()->add_global_attribute("TimeStamp", time_stamp);
	const attrs::named_scope scope;
	logging::core::get()->add_thread_attribute("Scope", scope);
}

void Logger::set_sink_filter()
{
	switch (s_severity_filter)
	{
	case PROD_WARN_ERR_LOGS:
		s_sink_pointer->set_filter(
			expr::attr<LogLevel>("Severity") <= ERR
		);
		break;
	case DEBUG_LOGS:
		s_sink_pointer->set_filter(
			expr::attr<LogLevel>("Severity") == DEBUG
		);
		break;
	case TRACE_LOGS:
		s_sink_pointer->set_filter(
			expr::attr<LogLevel>("Severity") == TRACE
		);
		break;
	default:
		break;
	}
}

boost::shared_ptr<sinks::synchronous_sink<sinks::text_ostream_backend>> Logger::set_sink()
{
	boost::shared_ptr<sinks::synchronous_sink<sinks::text_ostream_backend>> sink_point(
		new sinks::synchronous_sink<sinks::text_ostream_backend>);
	{
		const sinks::synchronous_sink<sinks::text_ostream_backend>::locked_backend_ptr backend_point = sink_point->
			locked_backend();
		const boost::shared_ptr<std::ostream> stream_point(&std::clog, boost::null_deleter());
		backend_point->add_stream(stream_point);
	}
	logging::core::get()->add_sink(sink_point);
	return sink_point;
}

void Logger::set_sink_formatter(
	const boost::shared_ptr<sinks::synchronous_sink<sinks::text_ostream_backend>>& sink_point)
{
	sink_point->set_formatter(expr::stream
		<< logging::expressions::attr<logging::attributes::current_thread_id::value_type>("ThreadID")
		<< " - " << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%d/%m/%Y %H:%M:%S.%f")
		<< " [" << expr::attr<LogLevel>("Severity")
		<< "] - ["
		<< format_named_scope("Scope", keywords::format = "%n", keywords::iteration = expr::reverse) << "] "
		<< expr::smessage
	);
}

void Logger::Setup(const int& severity = NO_LOGS)
{
	set_attributes();
	s_sink_pointer = set_sink();
	set_sink_formatter(s_sink_pointer);
	s_severity_filter = severity;
	set_sink_filter();
}

void Logger::Reset()
{
	logging::core::get()->remove_all_sinks();
	s_sink_pointer.reset();
}

void Logger::LogDebug(const std::string& message)
{
	BOOST_LOG_SCOPED_THREAD_ATTR("ThreadID", attrs::current_thread_id())
	BOOST_LOG_SEV(g_slg, LogLevel::DEBUG) << "\033[1;34m" << message << "\033[0m";
}

void Logger::LogTrace(const std::string& message)
{
	BOOST_LOG_SCOPED_THREAD_ATTR("ThreadID", attrs::current_thread_id())
	BOOST_LOG_SEV(g_slg, LogLevel::TRACE) << "\033[1;36m" << message << "\033[0m";
}

void Logger::LogProd(const std::string& message)
{
	BOOST_LOG_SCOPED_THREAD_ATTR("ThreadID", attrs::current_thread_id())
	BOOST_LOG_SEV(g_slg, LogLevel::PROD) << "\033[1;31m" << message << "\033[0m";
}

void Logger::LogWarning(const std::string& message)
{
	BOOST_LOG_SCOPED_THREAD_ATTR("ThreadID", attrs::current_thread_id())
	BOOST_LOG_SEV(g_slg, LogLevel::WARNING) << "\033[1;31m" << message << "\033[0m";
}

void Logger::LogError(const std::string& message)
{
	BOOST_LOG_SCOPED_THREAD_ATTR("ThreadID", attrs::current_thread_id())
	BOOST_LOG_SEV(g_slg, LogLevel::ERR) << "\033[1;31m" << message << "\033[0m";
}

Logger::~Logger()
{
	Reset();
}
