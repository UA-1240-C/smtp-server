#include "ServerConfig.h"

// Constructor that initializes the configuration from a file
Config::Config(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        // If file cannot be opened, use default values
        std::cerr << "Warning: Could not open file. Settings are set to default values" << std::endl;
    } else {
        try {
            std::stringstream buffer;
            buffer << file.rdbuf(); // Read the file content into a stringstream
            JSON json = JSON::Parse(buffer.str()); // Parse the JSON content

            ParseJSON(json); // Parse and set the configuration values
        } catch (const JSONParseException &e) {
            std::cerr << "Error: Failed to parse JSON file - " << e.what() << std::endl;
            std::cerr << "Using default configuration values instead." << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Error: An unexpected error occurred - " << e.what() << std::endl;
            std::cerr << "Using default configuration values instead." << std::endl;
        }
    }
}

Config::Server Config::get_server() const { return m_server; }
Config::CommunicationSettings Config::get_communication_settings() const { return m_communication_settings; }
Config::Logging Config::get_logging() const { return m_logging; }
Config::Time Config::get_time() const { return m_time; }
Config::ThreadPool Config::get_thread_pool() const { return m_thread_pool; }

void Config::ParseServerConfig(const JSON &root) {
    if (root.get_object_value().count("Server")) {
        const auto &server_obj = root.get_object_value().at("Server").get_object_value();
        m_server.m_server_name = GetValueOrDefault(server_obj, "servername", m_server.m_server_name);
        m_server.m_server_display_name = GetValueOrDefault(server_obj, "serverdisplayname", m_server.m_server_display_name);
        m_server.m_listener_port = GetValueOrDefault(server_obj, "listenerport", m_server.m_listener_port);
        m_server.m_ip_address = GetValueOrDefault(server_obj, "ipaddress", m_server.m_ip_address);
    } else {
        NotifyDefault("Server"); // Notify if Server configuration is missing
    }
}

void Config::ParseCommunicationSettings(const JSON &root) {
    if (root.get_object_value().count("communicationsettings")) {
        const auto &comm_settings_obj = root.get_object_value().at("communicationsettings").get_object_value();
        m_communication_settings.m_blocking = GetValueOrDefault(comm_settings_obj, "blocking", m_communication_settings.m_blocking);
        m_communication_settings.m_socket_timeout = GetValueOrDefault(comm_settings_obj, "socket_timeout", m_communication_settings.m_socket_timeout);
    } else {
        NotifyDefault("CommunicationSettings"); // Notify if CommunicationSettings configuration is missing
    }
}

void Config::ParseLoggingConfig(const JSON &root) {
    if (root.get_object_value().count("logging")) {
        const auto &logging_obj = root.get_object_value().at("logging").get_object_value();
        m_logging.m_filename = GetValueOrDefault(logging_obj, "filename", m_logging.m_filename);
        m_logging.m_log_level = GetValueOrDefault(logging_obj, "LogLevel", m_logging.m_log_level);
        m_logging.m_flush = GetValueOrDefault(logging_obj, "flush", m_logging.m_flush);
    } else {
        NotifyDefault("Logging"); // Notify if Logging configuration is missing
    }
}

void Config::ParseTimeConfig(const JSON &root) {
    if (root.get_object_value().count("time")) {
        const auto &time_obj = root.get_object_value().at("time").get_object_value();
        m_time.m_period_time = GetValueOrDefault(time_obj, "Period_time", m_time.m_period_time);
    } else {
        NotifyDefault("Time"); // Notify if Time configuration is missing
    }
}

void Config::ParseThreadPoolConfig(const JSON &root) {
    if (root.get_object_value().count("threadpool")) {
        const auto &thread_pool_obj = root.get_object_value().at("threadpool").get_object_value();
        m_thread_pool.m_max_working_threads = GetValueOrDefault(thread_pool_obj, "maxworkingthreads", m_thread_pool.m_max_working_threads);
    } else {
        NotifyDefault("ThreadPool"); // Notify if ThreadPool configuration is missing
    }
}

void Config::ParseJSON(const JSON &json) {
    const auto &root = json.get_object_value().at("root"); // Get the root object

    ParseServerConfig(root);
    ParseCommunicationSettings(root);
    ParseLoggingConfig(root);
    ParseTimeConfig(root);
    ParseThreadPoolConfig(root);
}

// Template method to get a value from the JSON object or use default
template<typename T>
T Config::GetValueOrDefault(const std::unordered_map<std::string, JSON> &obj, const std::string &key, T default_value) const {
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
void Config::NotifyDefault(const std::string &property) const {
    std::cout << "Warning: " << property << " is set to default value." << std::endl;
}