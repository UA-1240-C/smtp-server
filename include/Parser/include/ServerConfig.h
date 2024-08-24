#ifndef SERVERCONFIG_H
#define SERVERCONFIG_H

#include <iostream>
#include <string>
#include <fstream>
#include "JSONParser.h"

class Config {
public:
    struct Server {
        std::string m_server_name = "DefaultServer"; // Default server name
        std::string m_server_display_name = "DefaultServerDisplayName"; // Default display name
        int m_listener_port = 25000; // Default listener port
        std::string m_ip_address = "127.0.0.1"; // Default IP address
    };

    struct CommunicationSettings {
        int m_blocking = 0; // Default m_blocking setting
        int m_socket_timeout = 5; // Default socket timeout
    };

    struct Logging {
        std::string m_filename = "serverlog.txt"; // Default log file name
        int m_log_level = 2; // Default log level
        int m_flush = 0; // Default flush setting
    };

    struct Time {
        int m_period_time = 30; // Default period time
    };

    struct ThreadPool {
        int m_max_working_threads = 10; // Default max number of working threads
    };

    Config(const std::string &filename);

    Server get_server() const;
    CommunicationSettings get_communication_settings() const;
    Logging get_logging() const;
    Time get_time() const;
    ThreadPool get_thread_pool() const;

private:
    Server m_server;
    CommunicationSettings m_communication_settings;
    Logging m_logging;
    Time m_time;
    ThreadPool m_thread_pool;

    void ParseServerConfig(const JSON &root);
    void ParseCommunicationSettings(const JSON &root);
    void ParseLoggingConfig(const JSON &root);
    void ParseTimeConfig(const JSON &root);
    void ParseThreadPoolConfig(const JSON &root);
    void ParseJSON(const JSON &json);

    template<typename T>
    T GetValueOrDefault(const std::unordered_map<std::string, JSON> &obj, const std::string &key, T default_value) const;

    void NotifyDefault(const std::string &property) const;
};

#endif // CONFIG_H
