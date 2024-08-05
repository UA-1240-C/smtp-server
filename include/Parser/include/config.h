#include <iostream>
#include <string>
#include <fstream>
#include "parser.h"

class Config {
public:
    // Structs to hold different configuration settings
    struct Server {
        std::string servername = "DefaultServer"; // Default server name
        std::string serverdisplayname = "DefaultServerDisplayName"; // Default display name
        int listenerport = 25000; // Default listener port
        std::string ipaddress = "127.0.0.1"; // Default IP address
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
    Server get_Server() const { return server; }
    CommunicationSettings get_CommunicationSettings() const { return communication_settings; }
    Logging get_Logging() const { return logging; }
    Time get_Time() const { return time; }
    ThreadPool get_ThreadPool() const { return thread_pool; }

private:
    // Configuration members
    Server server;
    CommunicationSettings communication_settings;
    Logging logging;
    Time time;
    ThreadPool thread_pool;

    // Method to parse JSON and set configuration values
    void ParseJSON(const JSON &json) {
        const auto &root = json.get_Object_value().at("root"); // Get the root object

        // Accessing and setting Server configuration
        if (root.get_Object_value().count("Server")) {
            const auto &server_obj = root.get_Object_value().at("Server").get_Object_value();
            server.servername = GetValueOrDefault(server_obj, "servername", server.servername);
            server.serverdisplayname = GetValueOrDefault(server_obj, "serverdisplayname", server.serverdisplayname);
            server.listenerport = GetValueOrDefault(server_obj, "listenerport", server.listenerport);
            server.ipaddress = GetValueOrDefault(server_obj, "ipaddress", server.ipaddress);
        } else {
            NotifyDefault("Server"); // Notify if Server configuration is missing
        }

        // Accessing and setting CommunicationSettings
        if (root.get_Object_value().count("communicationsettings")) {
            const auto &comm_settings_obj = root.get_Object_value().at("communicationsettings").get_Object_value();
            communication_settings.blocking = GetValueOrDefault(comm_settings_obj, "blocking", communication_settings.blocking);
            communication_settings.socket_timeout = GetValueOrDefault(comm_settings_obj, "socket_timeout", communication_settings.socket_timeout);
        } else {
            NotifyDefault("CommunicationSettings"); // Notify if CommunicationSettings configuration is missing
        }

        // Accessing and setting Logging configuration
        if (root.get_Object_value().count("logging")) {
            const auto &logging_obj = root.get_Object_value().at("logging").get_Object_value();
            logging.filename = GetValueOrDefault(logging_obj, "filename", logging.filename);
            logging.log_level = GetValueOrDefault(logging_obj, "LogLevel", logging.log_level);
            logging.flush = GetValueOrDefault(logging_obj, "flush", logging.flush);
        } else {
            NotifyDefault("Logging"); // Notify if Logging configuration is missing
        }

        // Accessing and setting Time configuration
        if (root.get_Object_value().count("time")) {
            const auto &time_obj = root.get_Object_value().at("time").get_Object_value();
            time.period_time = GetValueOrDefault(time_obj, "Period_time", time.period_time);
        } else {
            NotifyDefault("Time"); // Notify if Time configuration is missing
        }

        // Accessing and setting ThreadPool configuration
        if (root.get_Object_value().count("threadpool")) {
            const auto &thread_pool_obj = root.get_Object_value().at("threadpool").get_Object_value();
            thread_pool.max_working_threads = GetValueOrDefault(thread_pool_obj, "maxworkingthreads", thread_pool.max_working_threads);
        } else {
            NotifyDefault("ThreadPool"); // Notify if ThreadPool configuration is missing
        }
    }

    // Template method to get a value from the JSON object or use default
    template<typename T>
    T GetValueOrDefault(const std::unordered_map<std::string, JSON> &obj, const std::string &key, T default_value) const {
        if (obj.count(key)) {
            const JSON &value_json = obj.at(key);
            if constexpr (std::is_same_v<T, std::string>) {
                return value_json.get_String_value();
            } else if constexpr (std::is_same_v<T, int>) {
                return static_cast<int>(value_json.get_Number_value());
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
