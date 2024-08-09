#include <iostream>
#include <string>
#include <fstream>
#include "parser.h"

class Config {
public:
    // Structs to hold different configuration settings
    struct Server {
        std::string server_name = "DefaultServer"; // Default server name
        std::string server_display_name = "DefaultServerDisplayName"; // Default display name
        int listener_port = 25000; // Default listener port
        std::string ip_address = "127.0.0.1"; // Default IP address
    };

    struct CommunicationSettings {
        int blocking = 0; // Default blocking setting
        int socket_timeout = 5; // Default socket timeout
    };

    struct Logging {
        std::string filename = "serverlog.txt"; // Default log file name
        int log_level = 2; // Default log level
        int flush = 0; // Default flush setting
    };

    struct Time {
        int period_time = 30; // Default period time
    };

    struct ThreadPool {
        int max_working_threads = 10; // Default max number of working threads
    };

    // Constructor that initializes the configuration from a file
    Config(const std::string &filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            // If file cannot be opened, use default values
            std::cerr << "Warning: Could not open filename. Settings are set to default values" << std::endl;
        } else {
            std::stringstream buffer;
            buffer << file.rdbuf(); // Read the file content into a stringstream
            JSON json = JSON::Parse(buffer.str()); // Parse the JSON content

            ParseJSON(json); // Parse and set the configuration values
        }
    }

    // Getters for configuration settings
    Server get_server() const { return server; }
    CommunicationSettings get_communication_settings() const { return communication_settings; }
    Logging get_logging() const { return logging; }
    Time get_time() const { return time; }
    ThreadPool get_thread_pool() const { return thread_pool; }

private:
    // Configuration members
    Server server;
    CommunicationSettings communication_settings;
    Logging logging;
    Time time;
    ThreadPool thread_pool;

    void ParseServerConfig(const JSON &root) {
        if (root.get_object_value().count("Server")) {
            const auto &server_obj = root.get_object_value().at("Server").get_object_value();
            server.server_name = GetValueOrDefault(server_obj, "servername", server.server_name);
            server.server_display_name = GetValueOrDefault(server_obj, "serverdisplayname", server.server_display_name);
            server.listener_port = GetValueOrDefault(server_obj, "listenerport", server.listener_port);
            server.ip_address = GetValueOrDefault(server_obj, "ipaddress", server.ip_address);
        } else {
            NotifyDefault("Server"); // Notify if Server configuration is missing
        }
    }

    void ParseCommunicationSettings(const JSON &root) {
        if (root.get_object_value().count("communicationsettings")) {
            const auto &comm_settings_obj = root.get_object_value().at("communicationsettings").get_object_value();
            communication_settings.blocking = GetValueOrDefault(comm_settings_obj, "blocking", communication_settings.blocking);
            communication_settings.socket_timeout = GetValueOrDefault(comm_settings_obj, "socket_timeout", communication_settings.socket_timeout);
        } else {
            NotifyDefault("CommunicationSettings"); // Notify if CommunicationSettings configuration is missing
        }
    }

    void ParseLoggingConfig(const JSON &root) {
        if (root.get_object_value().count("logging")) {
            const auto &logging_obj = root.get_object_value().at("logging").get_object_value();
            logging.filename = GetValueOrDefault(logging_obj, "filename", logging.filename);
            logging.log_level = GetValueOrDefault(logging_obj, "LogLevel", logging.log_level);
            logging.flush = GetValueOrDefault(logging_obj, "flush", logging.flush);
        } else {
            NotifyDefault("Logging"); // Notify if Logging configuration is missing
        }
    }

    void ParseTimeConfig(const JSON &root) {
        if (root.get_object_value().count("time")) {
            const auto &time_obj = root.get_object_value().at("time").get_object_value();
            time.period_time = GetValueOrDefault(time_obj, "Period_time", time.period_time);
        } else {
            NotifyDefault("Time"); // Notify if Time configuration is missing
        }
    }

    void ParseThreadPoolConfig(const JSON &root) {
        if (root.get_object_value().count("threadpool")) {
            const auto &thread_pool_obj = root.get_object_value().at("threadpool").get_object_value();
            thread_pool.max_working_threads = GetValueOrDefault(thread_pool_obj, "maxworkingthreads", thread_pool.max_working_threads);
        } else {
            NotifyDefault("ThreadPool"); // Notify if ThreadPool configuration is missing
        }
    }

    void ParseJSON(const JSON &json) {
        const auto &root = json.get_object_value().at("root"); // Get the root object

        ParseServerConfig(root);
        ParseCommunicationSettings(root);
        ParseLoggingConfig(root);
        ParseTimeConfig(root);
        ParseThreadPoolConfig(root);
    }

    // Template method to get a value from the JSON object or use default
    template<typename T>
    T GetValueOrDefault(const std::unordered_map<std::string, JSON> &obj, const std::string &key, T default_value) const {
        if (obj.count(key)) {
            const JSON &value_json = obj.at(key);
            if constexpr (std::is_same_v<T, std::string>) {
                return value_json.get_string_value();
            } else if constexpr (std::is_same_v<T, int>) {
                return static_cast<int>(value_json.get_number_value());
            }
        }
        NotifyDefault(key); // Notify if key is missing
        return default_value;
    }

    // Method to notify when a configuration property is set to default value
    void NotifyDefault(const std::string &property) const {
        std::cout << "Warning: " << property << " is set to default value." << std::endl;
    }
};
