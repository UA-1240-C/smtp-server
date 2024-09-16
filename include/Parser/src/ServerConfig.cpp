#include "ServerConfig.h"

// Constructor that initializes the configuration from a file
Config::Config(const std::string &filename) {
    Logger::LogDebug("Entering Config constructor");
    Logger::LogTrace("Constructor params: filename");
    Logger::LogDebug("Read the config file");
    std::ifstream file(filename);
    if (!file.is_open()) {
        Logger::LogWarning("Warning: Could not open file. Settings are set to default values");
    } else {
        Logger::LogProd("File read successfully.");
        try {
            std::stringstream buffer;
            buffer << file.rdbuf(); // Read the file content into a stringstream
            JSON json = JSON::Parse(buffer.str()); // Parse the JSON content

            ParseJSON(json); // Parse and set the configuration values
        } catch (const JSONParseException &e) {
            Logger::LogError("Error: Failed to parse JSON file - " + std::string(e.what()));
            Logger::LogWarning("Warning: Using default configuration values instead.");
        } catch (const std::exception &e) {
            Logger::LogError("Error: An unexpected error occurred - " + std::string(e.what()));
            Logger::LogWarning("Warning: Using default configuration values instead.");
        }
    }
    Logger::LogDebug("Exiting Config constructor");
}

Config::Server Config::get_server() const { return m_server; }
Config::CommunicationSettings Config::get_communication_settings() const { return m_communication_settings; }
Config::Logging Config::get_logging() const { return m_logging; }
Config::Time Config::get_time() const { return m_time; }
Config::ThreadPool Config::get_thread_pool() const { return m_thread_pool; }

void Config::ParseServerConfig(const JSON &root) {
    Logger::LogDebug("Entering ParseServerConfig");
    
    if (root.get_object_value().count("Server")) {
        Logger::LogTrace("Parsing Server configuration");
        const auto &server_obj = root.get_object_value().at("Server").get_object_value();
        m_server.m_server_name = GetValueOrDefault(server_obj, "servername", m_server.m_server_name);
        m_server.m_server_display_name = GetValueOrDefault(server_obj, "serverdisplayname", m_server.m_server_display_name);
        m_server.m_listener_port = GetValueOrDefault(server_obj, "listenerport", m_server.m_listener_port);
        m_server.m_ip_address = GetValueOrDefault(server_obj, "ipaddress", m_server.m_ip_address);
    } else {
        Logger::LogWarning("Server configuration not found. Using default values.");
    }

    Logger::LogDebug("Exiting ParseServerConfig");
}

void Config::ParseCommunicationSettings(const JSON &root) {
    Logger::LogDebug("Entering ParseCommunicationSettings");

    if (root.get_object_value().count("communicationsettings")) {
        Logger::LogTrace("Parsing CommunicationSettings configuration");
        const auto &comm_settings_obj = root.get_object_value().at("communicationsettings").get_object_value();
        m_communication_settings.m_blocking = GetValueOrDefault(comm_settings_obj, "blocking", m_communication_settings.m_blocking);
        m_communication_settings.m_socket_timeout = GetValueOrDefault(comm_settings_obj, "socket_timeout", m_communication_settings.m_socket_timeout);
    } else {
        Logger::LogWarning("CommunicationSettings configuration not found. Using default values.");
    }

    Logger::LogDebug("Exiting ParseCommunicationSettings");
}

void Config::ParseLoggingConfig(const JSON &root) {
    Logger::LogDebug("Entering ParseLoggingConfig");

    if (root.get_object_value().count("logging")) {
        Logger::LogTrace("Parsing Logging configuration");
        const auto &logging_obj = root.get_object_value().at("logging").get_object_value();
        m_logging.m_filename = GetValueOrDefault(logging_obj, "filename", m_logging.m_filename);
        m_logging.m_log_level = GetValueOrDefault(logging_obj, "LogLevel", m_logging.m_log_level);
        m_logging.m_flush = GetValueOrDefault(logging_obj, "flush", m_logging.m_flush);
    } else {
        Logger::LogWarning("Logging configuration not found. Using default values.");
    }

    Logger::LogDebug("Exiting ParseLoggingConfig");
}

void Config::ParseTimeConfig(const JSON &root) {
    Logger::LogDebug("Entering ParseTimeConfig");

    if (root.get_object_value().count("time")) {
        Logger::LogTrace("Parsing Time configuration");
        const auto &time_obj = root.get_object_value().at("time").get_object_value();
        m_time.m_period_time = GetValueOrDefault(time_obj, "Period_time", m_time.m_period_time);
    } else {
        Logger::LogWarning("Time configuration not found. Using default values.");
    }

    Logger::LogDebug("Exiting ParseTimeConfig");
}

void Config::ParseThreadPoolConfig(const JSON &root) {
    Logger::LogDebug("Entering ParseThreadPoolConfig");

    if (root.get_object_value().count("threadpool")) {
        Logger::LogTrace("Parsing ThreadPool configuration");
        const auto &thread_pool_obj = root.get_object_value().at("threadpool").get_object_value();
        m_thread_pool.m_max_working_threads = GetValueOrDefault(thread_pool_obj, "maxworkingthreads", m_thread_pool.m_max_working_threads);
    } else {
        Logger::LogWarning("ThreadPool configuration not found. Using default values.");
    }

    Logger::LogDebug("Exiting ParseThreadPoolConfig");
}

void Config::ParseJSON(const JSON &json) {
    Logger::LogDebug("Entering ParseJSON");

    const auto &root = json.get_object_value().at("root"); // Get the root object

    Logger::LogTrace("Parsing root object");
    ParseServerConfig(root);
    ParseCommunicationSettings(root);
    ParseLoggingConfig(root);
    ParseTimeConfig(root);
    ParseThreadPoolConfig(root);

    Logger::LogDebug("Exiting ParseJSON");
}

// Template method to get a value from the JSON object or use default
template<typename T>
T Config::GetValueOrDefault(const std::unordered_map<std::string, JSON> &obj, const std::string &key, T default_value) const {
    Logger::LogDebug("Entering GetValueOrDefault with key: " + key);

    if (obj.count(key)) {
        const JSON &value_json = obj.at(key);
        if constexpr (std::is_same_v<T, std::string>) {
            Logger::LogTrace("Found key: " + key + ", returning string value.");
            Logger::LogDebug("Exiting GetValueOrDefault with string value of " + key);
            return value_json.get_string_value();
        } else if constexpr (std::is_same_v<T, int>) {
            Logger::LogTrace("Found key: " + key + ", returning integer value.");
            Logger::LogDebug("Exiting GetValueOrDefault with integer value of " + key);
            return static_cast<int>(value_json.get_number_value());
        }
    }

    Logger::LogWarning("Key: " + key + " not found. Using default value.");
    Logger::LogDebug("Exiting GetValueOrDefault with default value.");
    return default_value;
}