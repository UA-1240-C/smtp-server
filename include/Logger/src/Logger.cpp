#include "Logger.h"

boost::shared_ptr<sinks::asynchronous_sink<sinks::text_ostream_backend>> Logger::s_sink_pointer;
logging::formatter Logger::s_sink_formatter;
uint8_t Logger::s_severity_filter;
std::ofstream Logger::s_log_file;
uint8_t Logger::s_flush;
boost::atomic<bool> Logger::is_running;
ISXThreadPool::ThreadSafeQueue<LogMessage> Logger::s_queue{};

const std::string Colors::BLUE = "\033[1;34m";
const std::string Colors::CYAN = "\033[1;36m";
const std::string Colors::RED = "\033[1;31m";
const std::string Colors::RESET = "\033[0m";

void Logger::set_attributes()
{
	const attrs::local_clock time_stamp;
	logging::core::get()->add_global_attribute("TimeStamp", time_stamp);
	const attrs::current_thread_id thread_id;
	logging::core::get()->add_global_attribute("ThreadID", thread_id);
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
	boost::shared_ptr<sinks::asynchronous_sink<sinks::text_ostream_backend>> console_sink_point(
		new sinks::asynchronous_sink<sinks::text_ostream_backend>);
	const sinks::asynchronous_sink<sinks::text_ostream_backend>::locked_backend_ptr console_backend_point =
		console_sink_point->locked_backend();
	const boost::shared_ptr<std::ostream> stream_point(&std::clog, boost::null_deleter());

	console_backend_point->add_stream(stream_point);
	console_backend_point->auto_flush(s_flush);

	console_sink_point->set_formatter
	(
		s_sink_formatter
	);
	logging::core::get()->add_sink(console_sink_point);
	return console_sink_point;
}

void Logger::set_sink_formatter()
{
	s_sink_formatter = expr::stream
		<< logging::expressions::attr<logging::attributes::current_thread_id::value_type>("ThreadID")
		<< " - " << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%d/%m/%Y %H:%M:%S.%f")
		<< " [" << boost::phoenix::bind(&Logger::SeverityToOutput)
		<< "] "
		<< expr::smessage;
}

void Logger::Setup(const Config::Logging& logging_config)
{
	s_severity_filter = static_cast<SeverityFilter>(logging_config.log_level);
	s_flush = static_cast<bool>(logging_config.flush);

	set_sink_formatter();
	s_sink_pointer = set_sink();
	set_attributes();
	set_sink_filter();
	is_running.store(true);

	std::thread queue_processing_thread(
		[]()
		{
			ProcessQueue(s_queue);
		});
	queue_processing_thread.detach();
}

void Logger::Reset()
{
	is_running.store(false);
	is_running.notify_all();
	boost::log::core::get()->flush();
	logging::core::get()->remove_all_sinks();
	s_sink_pointer.reset();
}

std::string Logger::SeverityToOutput()
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

void LogToConsole(const LogMessage& log_message)
{
	BOOST_LOG_SCOPED_THREAD_ATTR("ThreadID", attrs::current_thread_id())
	std::string color{};
	switch (log_message.log_level)
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
		BOOST_LOG_SEV(g_slg, log_message.log_level) << "- [" << log_message.location.function_name() << "] "
		<< color << log_message.message << Colors::RESET;
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
}

void LogToFile(const LogMessage& log_message)
{
	const std::thread::id thread_id = std::this_thread::get_id();
	std::ostringstream filename;

#if defined(_WIN32) || defined(_WIN64)
	filename << "\\serverlog_" << thread_id << ".txt";
#else
	filename << "/serverlog_" << thread_id << ".txt";
#endif

	std::string logfile_path = LOGFILE_PATH + filename.str();
	std::ofstream logfile;
	logfile.open(logfile_path, std::ios::app);

	if (logfile.is_open())
	{
		const std::string sev_level = Logger::SeverityToOutput();
		if (!logfile)
		{
			std::cerr << "Error opening file" << std::endl;
		}
		try
		{
			logfile <<
				thread_id <<
				" - " << boost::posix_time::second_clock::local_time() <<
				" [" << sev_level <<
				"] - [" << log_message.location.function_name() <<
				"] " << log_message.message << '\n';

			if (Logger::get_flush())
				Logger::s_log_file.flush();
		}
		catch (const std::exception& e)
		{
			std::cerr << e.what() << '\n';
		}
	}
}

void Logger::LogDebug(const std::string& message, const std::source_location& location)
{
	Log<DEBUG>(message, location);
}

void Logger::LogTrace(const std::string& message, const std::source_location& location)
{
	Log<TRACE>(message, location);
}

void Logger::LogProd(const std::string& message, const std::source_location& location)
{
	Log<PROD>(message, location);
}

void Logger::LogWarning(const std::string& message, const std::source_location& location)
{
	Log<WARNING>(message, location);
}

void Logger::LogError(const std::string& message, const std::source_location& location)
{
	Log<ERR>(message, location);
}

Logger::~Logger()
{
	boost::log::core::get()->flush();
}
