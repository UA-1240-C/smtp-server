#include "Logger.h"

void ISXLogger::SetAttributes()
{
	const attrs::local_clock time_stamp;
	logging::core::get()->add_global_attribute("TimeStamp", time_stamp);
	const attrs::named_scope scope;
	logging::core::get()->add_thread_attribute("Scope", scope);
}

void ISXLogger::SetSinkFilter()
{
	switch (severity_filter)
	{
	case PROD_WARN_ERR_LOGS:
		sink_pointer->set_filter(
			expr::attr<LogLevel>("Severity").or_default(WARNING) <= ERROR
		);
		break;
	case DEBUG_LOGS:
		sink_pointer->set_filter(
			expr::attr<LogLevel>("Severity").or_default(DEBUG) == DEBUG
		);
		break;
	case TRACE_LOGS:
		sink_pointer->set_filter(
			expr::attr<LogLevel>("Severity").or_default(TRACE) == TRACE
		);
		break;
	default:
		break;
	}
}

boost::shared_ptr<sinks::synchronous_sink<sinks::text_ostream_backend>> ISXLogger::SetSink()
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

void ISXLogger::SetSinkFormatter(
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

void ISXLogger::Setup(const int& severity = NO_LOGS)
{
	SetAttributes();
	sink_pointer = SetSink();
	SetSinkFormatter(sink_pointer);
	severity_filter = severity;
	SetSinkFilter();
}

void ISXLogger::Reset()
{
	logging::core::get()->remove_all_sinks();
	sink_pointer.reset();
}

void ISXLogger::Debug(const std::string& message)
{
	BOOST_LOG_SCOPED_THREAD_ATTR("ThreadID", attrs::current_thread_id())
	BOOST_LOG_SEV(slg, LogLevel::DEBUG) << "\033[1;34m" << message << "\033[0m";
}

void ISXLogger::Trace(const std::string& message)
{
	BOOST_LOG_FUNC()
	BOOST_LOG_SCOPED_THREAD_ATTR("ThreadID", attrs::current_thread_id())
	BOOST_LOG_SEV(slg, LogLevel::TRACE) << "\033[1;36m" << message << "\033[0m";
}

void ISXLogger::Prod(const std::string& message)
{
	BOOST_LOG_SCOPED_THREAD_ATTR("ThreadID", attrs::current_thread_id())
	BOOST_LOG_SEV(slg, LogLevel::PROD) << "\033[1;31m" << message << "\033[0m";
}

void ISXLogger::Warning(const std::string& message)
{
	BOOST_LOG_SCOPED_THREAD_ATTR("ThreadID", attrs::current_thread_id())
	BOOST_LOG_SEV(slg, LogLevel::WARNING) << "\033[1;31m" << message << "\033[0m";
}

void ISXLogger::Error(const std::string& message)
{
	BOOST_LOG_SCOPED_THREAD_ATTR("ThreadID", attrs::current_thread_id())
	BOOST_LOG_SEV(slg, LogLevel::ERROR) << "\033[1;31m" << message << "\033[0m";
}
