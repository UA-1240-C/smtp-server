#ifndef SERVER_INITIALIZER_H
#define SERVER_INITIALIZER_H

#include <iostream>
#include <memory>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "ServerConfig.h"
#include "ThreadPool.h"
#include "Logger.h"
#include "SignalHandler.h"

using boost::asio::ip::tcp;
using namespace ISXSignalHandler;

namespace ISXSS
{
	class ServerInitializer
	{
	public:
		ServerInitializer(boost::asio::io_context& io_context, boost::asio::ssl::context& ssl_context);

		std::string get_server_name();
		std::string get_server_display_name();
		unsigned get_port();
		size_t get_max_threads();
		auto get_timeout_seconds() -> std::chrono::seconds;
		uint8_t get_log_level();
		auto get_acceptor() -> std::unique_ptr<tcp::acceptor>;

		void set_server_name(std::string server_name);
		void set_server_display_name(std::string server_display_name);
		void set_port(unsigned port);
		void set_max_threads();
		void set_timeout_seconds(std::chrono::seconds timeout_seconds);
		void set_log_level(uint8_t log_level);
	private:
		void InitializeServerName();
		void InitializeLogging();
		void InitializeThreadPool();
		void InitializeAcceptor();
		void InitializeTimeout();

		ISXThreadPool::ThreadPool<> get_thread_pool_impl();
	private:
		boost::asio::io_context& m_io_context;
		boost::asio::ssl::context& m_ssl_context;
		Config m_config;
		std::unique_ptr<tcp::acceptor> m_acceptor;
		std::unique_ptr<ISXThreadPool::ThreadPool<>> m_thread_pool;

		std::string m_server_name;
		std::string m_server_display_name;
		unsigned m_port;

		size_t m_max_threads;

		boost::asio::steady_timer m_timeout_timer;
		std::chrono::seconds m_timeout_seconds;

		uint8_t m_log_level;
	};
}

#endif // SERVER_INITIALIZER_H
