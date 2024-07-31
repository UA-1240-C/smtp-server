#include "Logger.h"

LogPriority Logger::s_priority = INFO;

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
	std::string color{};
	std::string output_priority{};

	switch (user_priority)
	{
	case TRACE:
		color = "\033[1;36m"; // cyan
		output_priority = "TRACE";
		break;
	case DEBUG:
		color = "\033[1;34m"; // blue
		output_priority = "DEBUG";
		break;
	case PROD:
		color = "\033[1;35m"; // purple
		output_priority = "PROD";
		break;
	case INFO:
		color = "\033[1;32m"; // green
		output_priority = "INFO";
		break;
	case WARNING:
		color = "\033[1;33m"; // yellow
		output_priority = "WARNING";
		break;
	case ERROR:
		color = "\033[1;31m"; // red
		output_priority = "ERROR";
		break;
	default:
		color = "\033[1;37m";
		output_priority = "UNKNOWN";
		break;
	}

	PrintLogging(color, output_priority, function_name, message, file, line);
}

void Logger::PrintLogging(const std::string& color, const std::string& output_priority,
						const std::optional<std::string>& function_name, const std::string& message,
						const std::string& file, const int line)
{
	// TODO: convert time setting to use localtime_s for safety
	const auto now_time_point = std::chrono::system_clock::now();
	const std::time_t now_time = std::chrono::system_clock::to_time_t(now_time_point);
	const std::tm now = *std::localtime(&now_time);

	// TODO: some mode switch between file and console?
	// console output
	std::cout << color << std::put_time(&now, "%Y-%m-%d %H:%M:%S") << " [" << output_priority << "] ";
	if (function_name.has_value())
	{
		std::cout << "(" << function_name.value() << ") ";
	}
	if (!file.empty())
	{
		std::cout << file.substr(file.find("src")) << ":(" << line << ')' << ' ';
	}
	std::cout << message << "\033[0m" << '\n';
}
