#ifndef LOGGER_H
#define LOGGER_H

#pragma once
#include <iostream>
#include <iomanip>
#include <string>
#include <chrono>
#include <ctime>
#include <fstream>
#include <thread>
#include <mutex>

#define LOG(logger, message) ((logger).log((logger).getPriority(), message, __FILE__, __LINE__))
#define LOG_TRACE(logger, message) ((logger).log(logPriority::TRACE, message, __FILE__, __LINE__))
#define LOG_DEBUG(logger, message) ((logger).log(logPriority::DEBUG, message, __FILE__, __LINE__))
#define LOG_INFO(logger, message) ((logger).log(logPriority::INFO, message, __FILE__, __LINE__))
#define LOG_WARNING(logger, message) ((logger).log(logPriority::WARNING, message, __FILE__, __LINE__))
#define LOG_ERROR(logger, message) ((logger).log(logPriority::ERROR, message, __FILE__, __LINE__))
#define LOG_CRITICAL(logger, message) ((logger).log(logPriority::CRITICAL, message, __FILE__, __LINE__))

enum logPriority {
	TRACE,
	DEBUG,
	INFO,
	WARNING,
	ERROR,
	CRITICAL
};


class Logger {
private:
	static logPriority priority;
	std::mutex logMutex;

public:
	Logger();
	// TODO: define logger with params
	// explicit Logger(logPriority userPriority);
	~Logger() = default;
	static logPriority getPriority();
	static void setPriority(logPriority userPriority);
	void log(logPriority userPriority, const std::string &message, const std::string &file, int line);
};

#endif // LOGGER_H