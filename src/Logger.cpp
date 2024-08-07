#include "Logger.h"


std::string ISXLogger::set_log_file(const std::string& parser_filename)
{
	log_file = parser_filename;
	return log_file;
}

std::string ISXLogger::get_log_file()
{
	return log_file;
}

bool ISXLogger::set_flush(const int& parser_flush)
{
	flush = parser_flush;
	return flush;
}

bool ISXLogger::get_flush()
{
	return flush;
}

int ISXLogger::set_log_level(const int& parsing_log_level)
{
	log_level = parsing_log_level;
	return log_level;
}

src::severity_logger<LogLevel> ISXLogger::InitLogging(src::severity_logger<LogLevel>& slg)
{
	auto core = logging::core::get();

	using text_sink = sinks::synchronous_sink<sinks::text_ostream_backend>;
	const auto p_sink = boost::make_shared<text_sink>();

	{
		const text_sink::locked_backend_ptr p_backend = p_sink->locked_backend();
		const boost::shared_ptr<std::ostream> console_stream(&std::clog, boost::null_deleter());
		p_backend->add_stream(console_stream);

		const boost::shared_ptr<std::ofstream> file_stream(new std::ofstream("serverlog.txt"));
		assert(file_stream->is_open());
		p_backend->add_stream(file_stream);
	}

	core->add_global_attribute("ThreadID", attrs::current_thread_id());
	core->add_global_attribute("TimeStamp", attrs::local_clock());
	core->add_global_attribute("Severity", attrs::constant(log_level));
	core->add_global_attribute("Scope", attrs::named_scope());
	core->add_global_attribute("LineID", attrs::counter<unsigned int>());
	if (log_level == DEBUG_LVL)
	{
		core->add_global_attribute("Scope", attrs::named_scope());
	}
	p_sink->set_formatter(
		expr::format("<[%1%>[%2%] Scope: %3%:%4%")
		% expr::attr<attrs::current_thread_id::value_type>("ThreadID")
		% expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%d/%m/%Y %H:%M:%S")
		% format_named_scope("Scope", keywords::format = "%n", keywords::iteration = expr::reverse, keywords::depth = 2)
		% expr::smessage
	);

	logging::core::get()->add_sink(p_sink);
	return slg;
}

void ISXLogger::WarningLog(src::severity_logger<LogLevel>& slg, const std::string& message)
{
	std::lock_guard<std::mutex> lock(log_mutex);
	BOOST_LOG_SEV(slg, WARNING) << message;
}

void ISXLogger::ErrorLog(src::severity_logger<LogLevel>& slg, const std::string& message)
{
	std::lock_guard<std::mutex> lock(log_mutex);
	BOOST_LOG_SEV(slg, ERROR) << message;
}

void ISXLogger::DebugLog(src::severity_logger<LogLevel>& slg, const std::string& message)
{
	std::lock_guard<std::mutex> lock(log_mutex);
	BOOST_LOG_SEV(slg, DEBUG) << message;
}

void ISXLogger::TraceLog(src::severity_logger<LogLevel>& slg, const std::string& message)
{
	std::lock_guard<std::mutex> lock(log_mutex);
	BOOST_LOG_SEV(slg, TRACE) << message;
}
