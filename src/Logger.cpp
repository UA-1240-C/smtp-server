#include "Logger.h"

LogPriority Logger::s_priority = INFO;
std::mutex Logger::s_log_mutex;

LogPriority Logger::get_priority()
{
	return s_priority;
}

void Logger::set_priority(const LogPriority user_priority = INFO)
{
	s_priority = user_priority;
}

Logger::Logger()
{
	set_priority();
}

Logger::Logger(const LogPriority user_priority)
{
	set_priority(user_priority);
}


void Logger::Log(LogPriority user_priority, const std::optional<std::string>& function_name, const std::string& message,
				const std::string& file, int line)
{
	std::lock_guard<std::mutex> guard(s_log_mutex);
	std::string output_priority{};

	switch (user_priority)
	{
	case TRACE:
		output_priority = "TRACE";
		break;
	case DEBUG:
		output_priority = "DEBUG";
		break;
	case PROD:
		output_priority = "PROD";
		break;
	case INFO:
		output_priority = "INFO";
		break;
	case WARNING:
		output_priority = "WARNING";
		break;
	case ERROR:
		output_priority = "ERROR";
		break;
	default:
		output_priority = "UNKNOWN";
		break;
	}

	FileLogging(output_priority, function_name, message, file, line);
}

void Logger::FileLogging(const std::string& output_priority,
						const std::optional<std::string>& function_name, const std::string& message,
						const std::string& file, const int line)
{
	const auto now_time_point = std::chrono::system_clock::now();
	const std::time_t now_time = std::chrono::system_clock::to_time_t(now_time_point);
	std::tm now{};

#ifdef _WIN32
	localtime_s(&now, &now_time);
#else
	localtime_r(&now_time, &now);
#endif

	// file output
	std::ofstream log_file(SERVERLOG, std::ios_base::out | std::ios::app);
	if (!log_file.is_open())
	{
		std::cerr << "error while opening the " << SERVERLOG << "! bad!";
		return;
	}
	log_file << std::put_time(&now, "%Y-%m-%d %H:%M:%S") << " [" << output_priority << "] ";
	if (function_name.has_value())
	{
		log_file << "(" << function_name.value() << ") ";
	}
	if (!file.empty())
	{
		log_file << file.substr(file.find("src")) << ":(" << line << ')' << ' ';
	}
	log_file << message << '\n';
}
