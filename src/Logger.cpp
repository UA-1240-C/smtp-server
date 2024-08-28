#include "Logger.h"

boost::shared_ptr<sinks::synchronous_sink<sinks::text_ostream_backend>> Logger::s_sink_pointer;
uint8_t Logger::s_severity_filter;
std::string Logger::s_log_file;
bool Logger::s_flush;
std::mutex Logger::s_logging_mutex;
ISXThreadPool::ThreadPool<> Logger::s_thread_pool(MAX_THREAD_COUNT);

const std::string Colors::BLUE = "\033[1;34m";
const std::string Colors::CYAN = "\033[1;36m";
const std::string Colors::RED = "\033[1;31m";
const std::string Colors::RESET = "\033[0m";

void Logger::set_attributes()
{
	const attrs::local_clock time_stamp;
	logging::core::get()->add_global_attribute("TimeStamp", time_stamp);
	logging::core::get()->add_global_attribute("Scope", attrs::named_scope());
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
		LogError("Error while setting sink filter!");
		s_sink_pointer->set_filter(
			expr::attr<LogLevel>("Severity") > TRACE
		);
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
		<< " [" << expr::attr<LogLevel>("Severity")
		<< "] - ["
		<< format_named_scope("Scope", keywords::format = "%n", keywords::iteration = expr::reverse)
		<< "] "
		<< expr::smessage
	);
}

void Logger::Setup(const Config::Logging &logging_config)
{
	std::lock_guard<std::mutex> lock(s_logging_mutex);
	s_log_file = logging_config.filename;
	s_severity_filter = logging_config.log_level;
	s_flush = logging_config.flush ? true : false;

	s_sink_pointer = set_sink();
	set_attributes();
	set_sink_formatter();
	set_sink_filter();
}

void Logger::Reset()
{
	s_thread_pool.WaitForTasks();
	logging::core::get()->remove_all_sinks();
	s_sink_pointer.reset();
}

void Logger::LogToConsole(const std::string &message, const LogLevel &log_level)
{
	BOOST_LOG_SCOPED_THREAD_ATTR("ThreadID", attrs::current_thread_id())
		std::lock_guard<std::mutex> lock(s_logging_mutex);
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
	BOOST_LOG_SEV(g_slg, log_level) << color << message << Colors::RESET;
}

void Logger::LogDebug(const std::string &message)
{
	s_thread_pool.EnqueueDetach([message]()
		{
			LogToConsole(message, DEBUG);
		}
	);
}

void Logger::LogTrace(const std::string &message)
{
	s_thread_pool.EnqueueDetach([message]()
		{
			LogToConsole(message, TRACE);
		}
	);
}

void Logger::LogProd(const std::string &message)
{
	s_thread_pool.EnqueueDetach([message]()
		{
			LogToConsole(message, PROD);
		}
	);
}

void Logger::LogWarning(const std::string &message)
{
	s_thread_pool.EnqueueDetach([message]()
		{
			LogToConsole(message, WARNING);
		}
	);
}

void Logger::LogError(const std::string &message)
{
	s_thread_pool.EnqueueDetach([message]()
		{
			LogToConsole(message, ERR);
		}
	);
}

Logger::~Logger()
{
	s_thread_pool.WaitForTasks();
	Reset();
}
