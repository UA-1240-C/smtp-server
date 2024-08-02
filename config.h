#include <iostream>
#include <string>
#include <fstream>
#include "parser.h"

class Config {
public:
    struct Server {
        std::string servername = "DefaultServer";
        std::string serverdisplayname = "DefaultServerDisplayName";
        int listenerport = 25000;
        std::string ipaddress = "127.0.0.1";
    };

    struct CommunicationSettings {
        int blocking = 0;
        int socket_timeout = 5;
    };

    struct Logging {
        std::string filename = "serverlog.txt";
        int logLevel = 2;
        int flush = 0;
    };

    struct Time {
        int period_time = 30;
    };

    struct ThreadPool {
        int maxWorkingThreads = 10;
    };

    Config(const std::string &filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open filename. Settings is set to default values" << std::endl;
        } else {
            std::stringstream buffer;
            buffer << file.rdbuf();
            JSON json = JSON::parse(buffer.str());

            parseJSON(json);
        }
    }

    Server getServer() const { return server; }
    CommunicationSettings getCommunicationSettings() const { return communicationSettings; }
    Logging getLogging() const { return logging; }
    Time getTime() const { return time; }
    ThreadPool getThreadPool() const { return threadPool; }

private:
    Server server;
    CommunicationSettings communicationSettings;
    Logging logging;
    Time time;
    ThreadPool threadPool;

    void parseJSON(const JSON &json) {
        // Accessing the "root" object
        const auto &root = json.object_value.at("root");

        // Accessing Server configuration
        if (root.object_value.count("Server")) {
            const auto &serverObj = root.object_value.at("Server").object_value;
            server.servername = getValueOrDefault(serverObj, "servername", server.servername);
            server.serverdisplayname = getValueOrDefault(serverObj, "serverdisplayname", server.serverdisplayname);
            server.listenerport = getValueOrDefault(serverObj, "listenerport", server.listenerport);
            server.ipaddress = getValueOrDefault(serverObj, "ipaddress", server.ipaddress);
        } else {
            notifyDefault("Server");
        }

        // Accessing CommunicationSettings
        if (root.object_value.count("communicationsettings")) {
            const auto &commSettingsObj = root.object_value.at("communicationsettings").object_value;
            communicationSettings.blocking = getValueOrDefault(commSettingsObj, "blocking", communicationSettings.blocking);
            communicationSettings.socket_timeout = getValueOrDefault(commSettingsObj, "socket_timeout", communicationSettings.socket_timeout);
        } else {
            notifyDefault("CommunicationSettings");
        }

        // Accessing Logging
        if (root.object_value.count("logging")) {
            const auto &loggingObj = root.object_value.at("logging").object_value;
            logging.filename = getValueOrDefault(loggingObj, "filename", logging.filename);
            logging.logLevel = getValueOrDefault(loggingObj, "LogLevel", logging.logLevel);
            logging.flush = getValueOrDefault(loggingObj, "flush", logging.flush);
        } else {
            notifyDefault("Logging");
        }

        // Accessing Time
        if (root.object_value.count("time")) {
            const auto &timeObj = root.object_value.at("time").object_value;
            time.period_time = getValueOrDefault(timeObj, "Period_time", time.period_time);
        } else {
            notifyDefault("Time");
        }

        // Accessing ThreadPool
        if (root.object_value.count("threadpool")) {
            const auto &threadPoolObj = root.object_value.at("threadpool").object_value;
            threadPool.maxWorkingThreads = getValueOrDefault(threadPoolObj, "maxworkingthreads", threadPool.maxWorkingThreads);
        } else {
            notifyDefault("ThreadPool");
        }
    }

    template<typename T>
    T getValueOrDefault(const std::unordered_map<std::string, JSON> &obj, const std::string &key, T defaultValue) const {
        if (obj.count(key)) {
            const JSON &valueJSON = obj.at(key);
            if constexpr (std::is_same_v<T, std::string>) {
                return valueJSON.str_value;
            } else if constexpr (std::is_same_v<T, int>) {
                return static_cast<int>(valueJSON.num_value);
            }
        }
        notifyDefault(key);
        return defaultValue;
    }

    void notifyDefault(const std::string &property) const {
        std::cout << "Warning: " << property << " is set to default value." << std::endl;
    }
};
