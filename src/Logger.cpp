#include "Logger.h"

logPriority Logger::priority = logPriority::INFO;

logPriority Logger::getPriority() {
	return Logger::priority;
}

void Logger::setPriority(logPriority userPriority = INFO) {
	Logger::priority = userPriority;
}

Logger::Logger() {
	setPriority();
}

void Logger::log(logPriority userPriority, const std::string &message, const std::string &file, int line) {
	// TODO: convert time setting to use localtime_s for safety
	auto nowTimePoint = std::chrono::system_clock::now();
	std::time_t nowTime = std::chrono::system_clock::to_time_t(nowTimePoint);
	std::tm now = *std::localtime(&nowTime);


	std::string color{};
	std::string outputPriority{};

	switch (userPriority) {
	case logPriority::TRACE:
		color = "\033[0;36m"; // cyan
		outputPriority = "TRACE";
		break;
	case logPriority::DEBUG:
		color = "\033[0;34m"; // blue
		outputPriority = "DEBUG";
		break;
	case logPriority::INFO:
		color = "\033[0;32m"; // green
		outputPriority = "INFO";
		break;
	case logPriority::WARNING:
		color = "\033[0;33m"; // yellow
		outputPriority = "WARNING";
		break;
	case logPriority::ERROR:
		color = "\033[0;31m"; // red
		outputPriority = "ERROR";
		break;
	case logPriority::CRITICAL:
		color = "\033[0;35m"; // magenta
		outputPriority = "CRITICAL";
		break;
	default:
		color = "\033[0m";
		outputPriority = "UNKNOWN";
		break;
	}

	// console output
	std::cout << color << std::put_time(&now, "%Y-%m-%d %H:%M:%S") << " [" << outputPriority << "] " << message;
	if (!file.empty()) {
		std::cout << " [" << file << ":" << line << "]";
	}
	std::cout << "\033[0m" << '\n';

}