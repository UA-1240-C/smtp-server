#pragma once

#ifdef _MSC_VER
#define __FUNCTION_SIGNATURE__ __FUNCSIG__
#else
#define __FUNCTION_SIGNATURE__ __PRETTY_FUNCTION__
#endif


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
#include <optional>

// small problem here. if LOG is called with manually set TRACE priority on logger, there will be no function name
#define LOG(logger, message) ((logger).Log((logger).get_priority(), std::nullopt, message, __FILE__, __LINE__))
#define LOG_TRACE(logger, message) ((logger).Log(LogPriority::TRACE, __FUNCTION_SIGNATURE__, message, __FILE__, __LINE__))

// TODO: set auto start/end(+restart for server) functions logging (by scope?)
#define LOG_DEBUG(logger, message) ((logger).Log(LogPriority::DEBUG, std::nullopt, message, __FILE__, __LINE__))
#define LOG_PROD(logger, message) ((logger).Log(LogPriority::PROD, std::nullopt, message, __FILE__, __LINE__))

#define LOG_WARNING(logger, message) ((logger).Log(LogPriority::WARNING, std::nullopt, message, __FILE__, __LINE__))
#define LOG_ERROR(logger, message) ((logger).Log(LogPriority::ERROR, std::nullopt, message, __FILE__, __LINE__))

enum LogPriority
{
	TRACE,
	DEBUG,
	PROD,
	INFO,
	WARNING,
	ERROR
};


class Logger
{
	static LogPriority s_priority;

public:
	static std::mutex s_log_mutex;

	Logger();
	explicit Logger(LogPriority user_priority);
	~Logger() = default;
	static LogPriority get_priority();
	static void set_priority(LogPriority user_priority);
	void Log(LogPriority user_priority, const std::optional<std::string>& function_name, const std::string& message,
			const std::string& file, int line);
	void FileLogging(const std::string& output_priority,
					const std::optional<std::string>& function_name,
					const std::string& message, const std::string& file, int line);
};

#endif // LOGGER_H
