#include "Logger.h"

boost::shared_ptr<sinks::asynchronous_sink<sinks::text_ostream_backend>> Logger::s_sink_pointer;
uint8_t Logger::s_severity_filter;
std::string Logger::s_log_file;
uint8_t Logger::s_flush;
std::mutex Logger::s_logging_mutex;
thread_local std::unique_ptr<Logger> Logger::s_thread_local_logger;

const std::string Colors::BLUE = "\033[1;34m";
const std::string Colors::CYAN = "\033[1;36m";
const std::string Colors::RED = "\033[1;31m";
const std::string Colors::RESET = "\033[0m";

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
			expr::attr<LogLevel>("Severity") >= PROD
		);
		break;
	case DEBUG_LOGS:
		s_sink_pointer->set_filter(
			expr::attr<LogLevel>("Severity") >= DEBUG
		);
		break;
	case TRACE_LOGS:
		s_sink_pointer->set_filter(
			expr::attr<LogLevel>("Severity") >= TRACE
		);
		break;
	default:
		break;
	}
}

boost::shared_ptr<sinks::asynchronous_sink<sinks::text_ostream_backend>> Logger::set_sink()
{
	boost::shared_ptr<sinks::asynchronous_sink<sinks::text_ostream_backend>> sink_point(
		new sinks::asynchronous_sink<sinks::text_ostream_backend>);
	{
		const sinks::asynchronous_sink<sinks::text_ostream_backend>::locked_backend_ptr backend_point = sink_point->
			locked_backend();
		const boost::shared_ptr<std::ostream> stream_point(&std::clog, boost::null_deleter());
		backend_point->add_stream(stream_point);

		// TODO: configure file logging properly
		boost::shared_ptr<std::ofstream> file_stream(new std::ofstream(s_log_file));
		backend_point->add_stream(file_stream);

		s_flush
			? backend_point->auto_flush(true)
			: backend_point->auto_flush(false);
	}
	logging::core::get()->add_sink(sink_point);
	return sink_point;
}

void Logger::set_sink_formatter()
{
	s_sink_pointer->set_formatter(expr::stream
		<< logging::expressions::attr<logging::attributes::current_thread_id::value_type>("ThreadID")
		<< " - " << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%d/%m/%Y %H:%M:%S.%f")
		<< " [" << boost::phoenix::bind(&Logger::SeverityToOutput)
		<< "] "
		<< expr::smessage
	);
}

void Logger::Setup(const Config::Logging& logging_config)
{
	s_log_file = logging_config.filename;
	s_severity_filter = static_cast<SeverityFilter>(logging_config.log_level);
	s_flush = logging_config.flush;

	s_sink_pointer = set_sink();
	set_attributes();
	set_sink_formatter();
	set_sink_filter();
}

void Logger::Reset()
{
	boost::log::core::get()->flush();
	logging::core::get()->remove_all_sinks();
	s_sink_pointer.reset();
}

std::string Logger::SeverityToOutput() // maybe needs fixing for presision
{
	switch (s_severity_filter)
	{
	case NO_LOGS: return "";
	case PROD_WARN_ERR_LOGS: return "Prod/Warning/Error";
	case DEBUG_LOGS: return "Debug";
	case TRACE_LOGS: return "Trace";
	default: return "";
	}
}

void Logger::LogToConsole(const std::string& message, const LogLevel& log_level, const std::source_location& location)
{
	BOOST_LOG_SCOPED_THREAD_ATTR("ThreadID", attrs::current_thread_id())
	std::string color{};
	switch (log_level)
	{
	case PROD:
	case WARNING:
	case ERR:
		color = Colors::RED;
		break;
	case DEBUG:
		color = Colors::BLUE;
		break;
	case TRACE:
		color = Colors::CYAN;
		break;
	default:
		color = Colors::RESET;
		break;
	}
	try
	{
		std::lock_guard<std::mutex> lock(s_logging_mutex);
		BOOST_LOG_SEV(g_slg, log_level) << "- [" << location.function_name() << "] "
		<< color << message << Colors::RESET;
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
}

void Logger::LogDebug(const std::string& message, const std::source_location& location)
{
	if (!s_thread_local_logger)
	{
		s_thread_local_logger = std::make_unique<Logger>();
	}
	s_thread_local_logger->LogToConsole(message, DEBUG, location);
}

void Logger::LogTrace(const std::string& message, const std::source_location& location)
{
	if (!s_thread_local_logger)
	{
		s_thread_local_logger = std::make_unique<Logger>();
	}
	s_thread_local_logger->LogToConsole(message, TRACE, location);
}

void Logger::LogProd(const std::string& message, const std::source_location& location)
{
	if (!s_thread_local_logger)
	{
		s_thread_local_logger = std::make_unique<Logger>();
	}
	s_thread_local_logger->LogToConsole(message, PROD, location);
}

void Logger::LogWarning(const std::string& message, const std::source_location& location)
{
	if (!s_thread_local_logger)
	{
		s_thread_local_logger = std::make_unique<Logger>();
	}
	s_thread_local_logger->LogToConsole(message, WARNING, location);
}

void Logger::LogError(const std::string& message, const std::source_location& location)
{
	if (!s_thread_local_logger)
	{
		s_thread_local_logger = std::make_unique<Logger>();
	}
	s_thread_local_logger->LogToConsole(message, ERR, location);
}

Logger::~Logger()
{
	boost::log::core::get()->flush();
}
