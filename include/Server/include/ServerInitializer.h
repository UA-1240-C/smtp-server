#ifndef SERVER_INITIALIZER_H
#define SERVER_INITIALIZER_H

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
		[[nodiscard]] unsigned get_port() const;
		[[nodiscard]] size_t get_max_threads() const;
		[[nodiscard]] auto get_timeout_seconds() const -> std::chrono::seconds;
		[[nodiscard]] uint8_t get_log_level() const;
		[[nodiscard]] tcp::acceptor& get_acceptor() const;
		[[nodiscard]] auto get_thread_pool() const -> ISXThreadPool::ThreadPool<>&;
		[[nodiscard]] boost::asio::ssl::context& get_ssl_context() const;
		[[nodiscard]] boost::asio::io_context& get_io_context() const;
	private:
		void InitializeLogging();
		void InitializeThreadPool();
		void InitializeAcceptor();
		void InitializeTimeout();
	private:
		boost::asio::io_context& m_io_context;
		boost::asio::ssl::context& m_ssl_context;
		Config m_config;
		std::unique_ptr<tcp::acceptor> m_acceptor;
		std::unique_ptr<ISXThreadPool::ThreadPool<>> m_thread_pool;

		std::string m_server_name;
		std::string m_server_display_name;
		unsigned m_port;
		std::string m_server_ip;

		size_t m_max_threads;

		std::chrono::seconds m_timeout_seconds;

		uint8_t m_log_level;
	};
}

#endif // SERVER_INITIALIZER_H
