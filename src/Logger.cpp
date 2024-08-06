#include "../include/Logger.h"  // TODO: change to "Logger.h" through CMake;

std::mutex Logger::s_log_mutex;
// TODO: merge with parser to retrieve needed variables (m_current_log_level, m_flush, m_log_file)

Logger& Logger::get_logger()
{
	static Logger logger;
	return logger;
}

std::string Logger::set_log_file(const std::string& filename)
{
	m_log_file = filename;
	return m_log_file;
}

std::string Logger::get_log_file() const
{
	return m_log_file;
}

LogLevel Logger::set_log_level(const int& log_level)
{
	const boost::shared_ptr<logging::core> core = logging::core::get();
	switch (log_level)
	{
	case 0:
		m_current_log_level = {};
		core->set_filter(logging::trivial::severity > ERROR);
		break;
	case 1:
		m_current_log_level = PROD;
		core->set_filter(logging::trivial::severity >= PROD);
		break;
	case 2:
		m_current_log_level = DEBUG;
		core->set_filter(logging::trivial::severity == DEBUG);
		break;
	case 3:
		m_current_log_level = TRACE;
		core->set_filter(logging::trivial::severity == TRACE);
		break;
	default:
		m_current_log_level = {};
		core->set_filter(logging::trivial::severity > ERROR);
		break;
	}
	return m_current_log_level;
}

LogLevel Logger::get_log_level() const
{
	return m_current_log_level;
}

bool Logger::set_flush(const int& flush)
{
	m_flush = flush ? true : false;
	return m_flush;
}

bool Logger::get_flush() const
{
	return m_flush;
}


boost::shared_ptr<sink_ostream> Logger::init_logging()
{
	logging::add_file_log(
		keywords::file_name = get_logger().get_log_file(),
		keywords::format = "[%TimeStamp]: %Message%"
	);

	const boost::shared_ptr<logging::core> core = logging::core::get();
	logging::add_common_attributes(); // LineID, TimeStamp, ProcessID, ThreadID
	core->add_global_attribute("Severity", attrs::constant<LogLevel>(TRACE));
	BOOST_LOG_FUNCTION()

	if (get_logger().get_log_level() == DEBUG)
	{
		core->add_global_attribute("Scope", attrs::named_scope());
	}
	BOOST_LOG_TRIVIAL(info) << "Logger initialized";

	// Create a backend and initialize it with a stream
	const auto backend = boost::make_shared<sinks::text_ostream_backend>();
	backend->add_stream(boost::shared_ptr<std::ostream>(&std::clog, boost::null_deleter()));

	// Wrap it into the frontend and register in the core
	boost::shared_ptr<sink_ostream> sink(new sink_ostream(backend));
	core->add_sink(sink);
	sink->set_formatter(
		expr::stream
		<< expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%a %d/%m/%Y %H:%M:%S")
		<< " [" << get_logger().get_log_level() << "] "

		// TODO: fix this
		/*<< if_(expr::has_attr<std::string>("Scope"))
		[
			expr::stream << expr::format_named_scope(
				"Scope", "%a (%F:%l) ")
		]*/

		<< expr::message
	);


	// TODO: decide what to do with this scope thing from documentation
	// You can also manage backend in a thread-safe manner
	{
		std::lock_guard<std::mutex> log(s_log_mutex);
		const sink_ostream::locked_backend_ptr p = sink->locked_backend();
		p->add_stream(boost::make_shared<std::ofstream>());
	} // the backend gets released here

	return sink;
}

void Logger::stop_logging()
{
	logging::core::get()->remove_all_sinks();
	if (get_logger().get_flush())
	{
		logging::core::get()->flush();
	}
}
