#include "Logger.h"

BOOST_LOG_GLOBAL_LOGGER_INIT(g_logger, src::logger_mt)
{
	src::logger_mt _logger;
	return _logger;
}

std::mutex Logger::s_log_mutex;
// TODO: merge with parser to retrieve needed variables (m_log_level, m_flush, m_log_file)

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
		m_log_level = {};
		core->set_filter(logging::trivial::severity > ERROR);
		break;
	case 1:
		m_log_level = PROD;
		core->set_filter(logging::trivial::severity >= PROD);
		break;
	case 2:
		m_log_level = DEBUG;
		core->set_filter(logging::trivial::severity == DEBUG);
		break;
	case 3:
		m_log_level = TRACE;
		core->set_filter(logging::trivial::severity == TRACE);
		break;
	default:
		m_log_level = {};
		core->set_filter(logging::trivial::severity > ERROR);
		break;
	}
	return m_log_level;
}

LogLevel Logger::get_log_level() const
{
	return m_log_level;
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

boost::shared_ptr<sink_ostream> Logger::set_file_sink(const boost::shared_ptr<sink_ostream>& file_sink)
{
	m_file_sink = file_sink;
	return m_file_sink;
}

boost::shared_ptr<sink_ostream> Logger::get_file_sink() const
{
	return m_file_sink;
}

src::severity_logger_mt<LogLevel> Logger::get_m_logger() const
{
	return m_logger;
}

boost::shared_ptr<sink_ostream> Logger::InitLogging()
{
	const boost::shared_ptr<logging::core> core = logging::core::get();
	logging::add_common_attributes(); // LineID, TimeStamp, ProcessID, ThreadID
	core->add_global_attribute("Severity", attrs::constant(TRACE));
	core->add_global_attribute("TimeStamp", attrs::local_clock());
	core->add_global_attribute("RecordID", attrs::counter<unsigned int>());

	if (get_logger().get_log_level() == DEBUG)
	{
		core->add_global_attribute("Scope", attrs::named_scope());
	}

	const auto console_backend = boost::make_shared<sinks::text_ostream_backend>();
	console_backend->add_stream(boost::shared_ptr<std::ostream>(&std::clog, boost::null_deleter()));
	console_backend->auto_flush(get_logger().get_flush());

	const boost::shared_ptr<sink_ostream> console_sink(new sink_ostream(console_backend));
	// TODO: add formatter
	core->add_sink(console_sink);

	boost::shared_ptr<sink_ostream> file_sink(new sink_ostream(
		boost::make_shared<sinks::text_ostream_backend>()));

	core->add_sink(file_sink);
	get_logger().set_file_sink(file_sink);

	BOOST_LOG_TRIVIAL(info) << "Logger initialized";
	return file_sink;
}

void Logger::WriteLog(LogLevel, const std::string& function_name, const std::string& file, const int& line,
					const std::string& message)
{
	const boost::shared_ptr<logging::core> core = logging::core::get();
	std::lock_guard<std::mutex> guard(s_log_mutex);
	BOOST_LOG_SCOPED_THREAD_TAG("ThreadID", boost::this_thread::get_id())
	BOOST_LOG_SCOPED_THREAD_TAG("LineID", line)
	BOOST_LOG_SCOPED_THREAD_TAG("Scope", function_name)
	BOOST_LOG_SCOPED_THREAD_TAG("Severity", TRACE)
	BOOST_LOG_SCOPED_THREAD_TAG("TimeStamp", boost::posix_time::second_clock::local_time())
	BOOST_LOG_SCOPED_THREAD_TAG("RecordID", 0)

	boost::shared_ptr<std::ostream> strm(new std::ofstream(get_logger().get_log_file()));
	if (!strm->good())
		throw std::runtime_error("Failed to open a text log file");
	get_logger().get_file_sink()->locked_backend()->add_stream(strm);
	get_logger().get_file_sink()->set_formatter
	(
		expr::format("%1%: <%2%> %3%")
		% expr::attr<unsigned int>("LineID")
		% logging::trivial::severity
		% expr::message
	);
	BOOST_LOG_SEV(get_logger().get_m_logger(), get_logger().get_log_level()) << message;
	if (get_logger().get_flush())
		core->flush();
}


void Logger::StopLogging()
{
	logging::core::get()->remove_all_sinks();
	if (get_logger().get_flush())
	{
		logging::core::get()->flush();
	}
	BOOST_LOG_TRIVIAL(info) << "Logger stopped";
}
