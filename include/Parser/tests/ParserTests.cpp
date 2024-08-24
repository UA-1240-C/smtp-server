#include <gtest/gtest.h>
#include "ServerConfig.h"

TEST(JSONTest, ParseString) {
    JSON json = JSON::Parse("\"hello\"");
    EXPECT_EQ(json.get_type(), JSON::STRING);
    EXPECT_EQ(json.get_string_value(), "hello");
}

TEST(JSONTest, ParseNumber) {
    JSON json = JSON::Parse("123.45");
    EXPECT_EQ(json.get_type(), JSON::NUMBER);
    EXPECT_EQ(json.get_number_value(), 123.45);
}

TEST(JSONTest, ParseBoolTrue) {
    JSON json = JSON::Parse("true");
    EXPECT_EQ(json.get_type(), JSON::BOOL);
    EXPECT_EQ(json.get_bool_value(), true);
}

TEST(JSONTest, ParseBoolFalse) {
    JSON json = JSON::Parse("false");
    EXPECT_EQ(json.get_type(), JSON::BOOL);
    EXPECT_EQ(json.get_bool_value(), false);
}

TEST(JSONTest, ParseNull) {
    JSON json = JSON::Parse("null");
    EXPECT_EQ(json.get_type(), JSON::NIL);
}

TEST(JSONTest, ParseEmptyObject) {
    JSON json = JSON::Parse("{}");
    EXPECT_EQ(json.get_type(), JSON::OBJECT);
    EXPECT_TRUE(json.get_object_value().empty());
}

TEST(JSONTest, ParseObjectWithSingleKeyValuePair) {
    JSON json = JSON::Parse("{\"key\": \"value\"}");
    EXPECT_EQ(json.get_type(), JSON::OBJECT);
    ASSERT_EQ(json.get_object_value().size(), 1);
    EXPECT_EQ(json.get_object_value().at("key").get_type(), JSON::STRING);
    EXPECT_EQ(json.get_object_value().at("key").get_string_value(), "value");
}

TEST(JSONTest, ParseObjectWithMultipleKeyValuePairs) {
    JSON json = JSON::Parse("{\"key1\": \"value1\", \"key2\": 42, \"key3\": true}");
    EXPECT_EQ(json.get_type(), JSON::OBJECT);
    ASSERT_EQ(json.get_object_value().size(), 3);
    EXPECT_EQ(json.get_object_value().at("key1").get_type(), JSON::STRING);
    EXPECT_EQ(json.get_object_value().at("key1").get_string_value(), "value1");
    EXPECT_EQ(json.get_object_value().at("key2").get_type(), JSON::NUMBER);
    EXPECT_EQ(json.get_object_value().at("key2").get_number_value(), 42);
    EXPECT_EQ(json.get_object_value().at("key3").get_type(), JSON::BOOL);
    EXPECT_EQ(json.get_object_value().at("key3").get_bool_value(), true);
}

TEST(JSONTest, ParseSimpleServerConfig) {
    std::string jsonString = R"({
        "server": {
            "name": "SimpleServer",
            "port": 8080,
            "host": "192.168.1.1",
            "active": true
        },
        "database": {
            "type": "SQL",
            "user": "admin",
            "password": "secret"
        },
        "logging": {
            "level": "info",
            "file": "log.txt"
        }
    })";

    JSON json = JSON::Parse(jsonString);

    // Check server details
    ASSERT_EQ(json.get_type(), JSON::OBJECT);
    ASSERT_EQ(json.get_object_value().at("server").get_type(), JSON::OBJECT);
    EXPECT_EQ(json.get_object_value().at("server").get_object_value().at("name").get_type(), JSON::STRING);
    EXPECT_EQ(json.get_object_value().at("server").get_object_value().at("name").get_string_value(), "SimpleServer");
    EXPECT_EQ(json.get_object_value().at("server").get_object_value().at("port").get_type(), JSON::NUMBER);
    EXPECT_EQ(json.get_object_value().at("server").get_object_value().at("port").get_number_value(), 8080);
    EXPECT_EQ(json.get_object_value().at("server").get_object_value().at("host").get_type(), JSON::STRING);
    EXPECT_EQ(json.get_object_value().at("server").get_object_value().at("host").get_string_value(), "192.168.1.1");
    EXPECT_EQ(json.get_object_value().at("server").get_object_value().at("active").get_type(), JSON::BOOL);
    EXPECT_EQ(json.get_object_value().at("server").get_object_value().at("active").get_bool_value(), true);

    // Check database details
    ASSERT_EQ(json.get_object_value().at("database").get_type(), JSON::OBJECT);
    EXPECT_EQ(json.get_object_value().at("database").get_object_value().at("type").get_type(), JSON::STRING);
    EXPECT_EQ(json.get_object_value().at("database").get_object_value().at("type").get_string_value(), "SQL");
    EXPECT_EQ(json.get_object_value().at("database").get_object_value().at("user").get_type(), JSON::STRING);
    EXPECT_EQ(json.get_object_value().at("database").get_object_value().at("user").get_string_value(), "admin");
    EXPECT_EQ(json.get_object_value().at("database").get_object_value().at("password").get_type(), JSON::STRING);
    EXPECT_EQ(json.get_object_value().at("database").get_object_value().at("password").get_string_value(), "secret");

    // Check logging details
    ASSERT_EQ(json.get_object_value().at("logging").get_type(), JSON::OBJECT);
    EXPECT_EQ(json.get_object_value().at("logging").get_object_value().at("level").get_type(), JSON::STRING);
    EXPECT_EQ(json.get_object_value().at("logging").get_object_value().at("level").get_string_value(), "info");
    EXPECT_EQ(json.get_object_value().at("logging").get_object_value().at("file").get_type(), JSON::STRING);
    EXPECT_EQ(json.get_object_value().at("logging").get_object_value().at("file").get_string_value(), "log.txt");
}

TEST(JSONTest, ParseNestedConfig) {
    std::string jsonString = R"({
        "config": {
            "version": "1.0.0",
            "features": {
                "authentication": {
                    "enabled": true,
                    "method": "OAuth2"
                },
                "encryption": {
                    "enabled": true,
                    "type": "AES-256"
                }
            },
            "limits": {
                "max_users": 1000,
                "max_connections": 100
            }
        }
    })";

    JSON json = JSON::Parse(jsonString);

    // Check config details
    ASSERT_EQ(json.get_type(), JSON::OBJECT);
    ASSERT_EQ(json.get_object_value().at("config").get_type(), JSON::OBJECT);
    EXPECT_EQ(json.get_object_value().at("config").get_object_value().at("version").get_type(), JSON::STRING);
    EXPECT_EQ(json.get_object_value().at("config").get_object_value().at("version").get_string_value(), "1.0.0");

    // Check features details
    ASSERT_EQ(json.get_object_value().at("config").get_object_value().at("features").get_type(), JSON::OBJECT);
    ASSERT_EQ(json.get_object_value().at("config").get_object_value().at("features").get_object_value().at("authentication").get_type(), JSON::OBJECT);
    EXPECT_EQ(json.get_object_value().at("config").get_object_value().at("features").get_object_value().at("authentication").get_object_value().at("enabled").get_type(), JSON::BOOL);
    EXPECT_EQ(json.get_object_value().at("config").get_object_value().at("features").get_object_value().at("authentication").get_object_value().at("enabled").get_bool_value(), true);
    EXPECT_EQ(json.get_object_value().at("config").get_object_value().at("features").get_object_value().at("authentication").get_object_value().at("method").get_type(), JSON::STRING);
    EXPECT_EQ(json.get_object_value().at("config").get_object_value().at("features").get_object_value().at("authentication").get_object_value().at("method").get_string_value(), "OAuth2");

    ASSERT_EQ(json.get_object_value().at("config").get_object_value().at("features").get_object_value().at("encryption").get_type(), JSON::OBJECT);
    EXPECT_EQ(json.get_object_value().at("config").get_object_value().at("features").get_object_value().at("encryption").get_object_value().at("enabled").get_type(), JSON::BOOL);
    EXPECT_EQ(json.get_object_value().at("config").get_object_value().at("features").get_object_value().at("encryption").get_object_value().at("enabled").get_bool_value(), true);
    EXPECT_EQ(json.get_object_value().at("config").get_object_value().at("features").get_object_value().at("encryption").get_object_value().at("type").get_type(), JSON::STRING);
    EXPECT_EQ(json.get_object_value().at("config").get_object_value().at("features").get_object_value().at("encryption").get_object_value().at("type").get_string_value(), "AES-256");

    // Check limits details
    ASSERT_EQ(json.get_object_value().at("config").get_object_value().at("limits").get_type(), JSON::OBJECT);
    EXPECT_EQ(json.get_object_value().at("config").get_object_value().at("limits").get_object_value().at("max_users").get_type(), JSON::NUMBER);
    EXPECT_EQ(json.get_object_value().at("config").get_object_value().at("limits").get_object_value().at("max_users").get_number_value(), 1000);
    EXPECT_EQ(json.get_object_value().at("config").get_object_value().at("limits").get_object_value().at("max_connections").get_type(), JSON::NUMBER);
    EXPECT_EQ(json.get_object_value().at("config").get_object_value().at("limits").get_object_value().at("max_connections").get_number_value(), 100);
}

TEST(JSONTest, ParseComplexServerConfig) {
    std::string jsonString = R"({
        "root": {
            "Server": {
                "servername": "ServTest",
                "serverdisplayname": "ServTestserver",
                "listenerport": 25000,
                "ipaddress": "127.0.0.1"
            },
            "communicationsettings": {
                "blocking": 0,
                "socket_timeout": 5
            },
            "logging": {
                "filename": "serverlog.txt",
                "LogLevel": 2,
                "flush": 0
            },
            "time": {
                "Period_time": 30
            },
            "threadpool": {
                "maxworkingthreads": 10
            }
        }
    })";

    JSON json = JSON::Parse(jsonString);

    // Check root object
    ASSERT_EQ(json.get_type(), JSON::OBJECT);
    ASSERT_EQ(json.get_object_value().at("root").get_type(), JSON::OBJECT);

    // Check Server object
    ASSERT_EQ(json.get_object_value().at("root").get_object_value().at("Server").get_type(), JSON::OBJECT);
    EXPECT_EQ(json.get_object_value().at("root").get_object_value().at("Server").get_object_value().at("servername").get_type(), JSON::STRING);
    EXPECT_EQ(json.get_object_value().at("root").get_object_value().at("Server").get_object_value().at("servername").get_string_value(), "ServTest");
    EXPECT_EQ(json.get_object_value().at("root").get_object_value().at("Server").get_object_value().at("serverdisplayname").get_type(), JSON::STRING);
    EXPECT_EQ(json.get_object_value().at("root").get_object_value().at("Server").get_object_value().at("serverdisplayname").get_string_value(), "ServTestserver");
    EXPECT_EQ(json.get_object_value().at("root").get_object_value().at("Server").get_object_value().at("listenerport").get_type(), JSON::NUMBER);
    EXPECT_EQ(json.get_object_value().at("root").get_object_value().at("Server").get_object_value().at("listenerport").get_number_value(), 25000);
    EXPECT_EQ(json.get_object_value().at("root").get_object_value().at("Server").get_object_value().at("ipaddress").get_type(), JSON::STRING);
    EXPECT_EQ(json.get_object_value().at("root").get_object_value().at("Server").get_object_value().at("ipaddress").get_string_value(), "127.0.0.1");

    // Check communicationsettings object
    ASSERT_EQ(json.get_object_value().at("root").get_object_value().at("communicationsettings").get_type(), JSON::OBJECT);
    EXPECT_EQ(json.get_object_value().at("root").get_object_value().at("communicationsettings").get_object_value().at("blocking").get_type(), JSON::NUMBER);
    EXPECT_EQ(json.get_object_value().at("root").get_object_value().at("communicationsettings").get_object_value().at("blocking").get_number_value(), 0);
    EXPECT_EQ(json.get_object_value().at("root").get_object_value().at("communicationsettings").get_object_value().at("socket_timeout").get_type(), JSON::NUMBER);
    EXPECT_EQ(json.get_object_value().at("root").get_object_value().at("communicationsettings").get_object_value().at("socket_timeout").get_number_value(), 5);

    // Check logging object
    ASSERT_EQ(json.get_object_value().at("root").get_object_value().at("logging").get_type(), JSON::OBJECT);
    EXPECT_EQ(json.get_object_value().at("root").get_object_value().at("logging").get_object_value().at("filename").get_type(), JSON::STRING);
    EXPECT_EQ(json.get_object_value().at("root").get_object_value().at("logging").get_object_value().at("filename").get_string_value(), "serverlog.txt");
    EXPECT_EQ(json.get_object_value().at("root").get_object_value().at("logging").get_object_value().at("LogLevel").get_type(), JSON::NUMBER);
    EXPECT_EQ(json.get_object_value().at("root").get_object_value().at("logging").get_object_value().at("LogLevel").get_number_value(), 2);
    EXPECT_EQ(json.get_object_value().at("root").get_object_value().at("logging").get_object_value().at("flush").get_type(), JSON::NUMBER);
    EXPECT_EQ(json.get_object_value().at("root").get_object_value().at("logging").get_object_value().at("flush").get_number_value(), 0);

    // Check time object
    ASSERT_EQ(json.get_object_value().at("root").get_object_value().at("time").get_type(), JSON::OBJECT);
    EXPECT_EQ(json.get_object_value().at("root").get_object_value().at("time").get_object_value().at("Period_time").get_type(), JSON::NUMBER);
    EXPECT_EQ(json.get_object_value().at("root").get_object_value().at("time").get_object_value().at("Period_time").get_number_value(), 30);

    // Check threadpool object
    ASSERT_EQ(json.get_object_value().at("root").get_object_value().at("threadpool").get_type(), JSON::OBJECT);
    EXPECT_EQ(json.get_object_value().at("root").get_object_value().at("threadpool").get_object_value().at("maxworkingthreads").get_type(), JSON::NUMBER);
    EXPECT_EQ(json.get_object_value().at("root").get_object_value().at("threadpool").get_object_value().at("maxworkingthreads").get_number_value(), 10);
}


TEST(ConfigTest, DefaultValues) {
    Config config("non_existing_file.json");
    
    auto server = config.get_server();
    EXPECT_EQ(server.m_server_name, "DefaultServer");
    EXPECT_EQ(server.m_server_display_name, "DefaultServerDisplayName");
    EXPECT_EQ(server.m_listener_port, 25000);
    EXPECT_EQ(server.m_ip_address, "127.0.0.1");

    auto commSettings = config.get_communication_settings();
    EXPECT_EQ(commSettings.m_blocking, 0);
    EXPECT_EQ(commSettings.m_socket_timeout, 5);

    auto logging = config.get_logging();
    EXPECT_EQ(logging.m_filename, "serverlog.txt");
    EXPECT_EQ(logging.m_log_level, 2);
    EXPECT_EQ(logging.m_flush, 0);

    auto time = config.get_time();
    EXPECT_EQ(time.m_period_time, 30);

    auto threadPool = config.get_thread_pool();
    EXPECT_EQ(threadPool.m_max_working_threads, 10);
}

TEST(ConfigTest, ParseValidConfig) {
    const std::string jsonConfig = R"({
        "root": {
            "Server": {
                "servername": "TestServer",
                "serverdisplayname": "TestServerDisplayName",
                "listenerport": 30000,
                "ipaddress": "192.168.1.1"
            },
            "communicationsettings": {
                "blocking": 1,
                "socket_timeout": 10
            },
            "logging": {
                "filename": "testlog.txt",
                "LogLevel": 3,
                "flush": 1
            },
            "time": {
                "Period_time": 60
            },
            "threadpool": {
                "maxworkingthreads": 20
            }
        }
    })";

    // Write to a temporary file
    std::ofstream tempFile("temp_config.json");
    tempFile << jsonConfig;
    tempFile.close();

    Config config("temp_config.json");
    
    auto server = config.get_server();
    EXPECT_EQ(server.m_server_name, "TestServer");
    EXPECT_EQ(server.m_server_display_name, "TestServerDisplayName");
    EXPECT_EQ(server.m_listener_port, 30000);
    EXPECT_EQ(server.m_ip_address, "192.168.1.1");

    auto commSettings = config.get_communication_settings();
    EXPECT_EQ(commSettings.m_blocking, 1);
    EXPECT_EQ(commSettings.m_socket_timeout, 10);

    auto logging = config.get_logging();
    EXPECT_EQ(logging.m_filename, "testlog.txt");
    EXPECT_EQ(logging.m_log_level, 3);
    EXPECT_EQ(logging.m_flush, 1);

    auto time = config.get_time();
    EXPECT_EQ(time.m_period_time, 60);

    auto threadPool = config.get_thread_pool();
    EXPECT_EQ(threadPool.m_max_working_threads, 20);

    // Clean up the temporary file
    std::remove("temp_config.json");
}

TEST(ConfigTest, MissingFieldsUseDefaults) {
    const std::string jsonConfig = R"({
        "root": {
            "Server": {
                "servername": "TestServerName"
            }
        }
    })";

    // Write to a temporary file
    std::ofstream tempFile("temp_config.json");
    tempFile << jsonConfig;
    tempFile.close();

    Config config("temp_config.json");
    
    auto server = config.get_server();
    EXPECT_EQ(server.m_server_name, "TestServerName");
    EXPECT_EQ(server.m_server_display_name, "DefaultServerDisplayName");
    EXPECT_EQ(server.m_listener_port, 25000);
    EXPECT_EQ(server.m_ip_address, "127.0.0.1");

    auto commSettings = config.get_communication_settings();
    EXPECT_EQ(commSettings.m_blocking, 0);
    EXPECT_EQ(commSettings.m_socket_timeout, 5);

    auto logging = config.get_logging();
    EXPECT_EQ(logging.m_filename, "serverlog.txt");
    EXPECT_EQ(logging.m_log_level, 2);
    EXPECT_EQ(logging.m_flush, 0);

    auto time = config.get_time();
    EXPECT_EQ(time.m_period_time, 30);

    auto threadPool = config.get_thread_pool();
    EXPECT_EQ(threadPool.m_max_working_threads, 10);

    // Clean up the temporary file
    std::remove("temp_config.json");
}

// Tests for exeptions
TEST(JSONExeptionTest, ParseInvalidJSON) {
    EXPECT_THROW(JSON::Parse("{"), JSONParseException);
    EXPECT_THROW(JSON::Parse("}"), JSONParseException);
    EXPECT_THROW(JSON::Parse("["), JSONParseException);
    EXPECT_THROW(JSON::Parse("]"), JSONParseException);
    EXPECT_THROW(JSON::Parse("{]"), JSONParseException);
    EXPECT_THROW(JSON::Parse("{]"), JSONParseException);
    EXPECT_THROW(JSON::Parse("{\"key\":}"), JSONParseException);
}


TEST(JSONExeptionTest, InvalidCharacterException) {
    std::string invalidJson = "invalid";
    EXPECT_THROW({
        try {
            JSON::Parse(invalidJson);
        } catch (const JSONParseException& e) {
            EXPECT_STREQ("JSON Parse Error: Unexpected character", e.what());
            throw;
        }
    }, JSONParseException);
}

TEST(JSONExeptionTest, UnterminatedStringException) {
    std::string invalidJson = "\"Unterminated string";
    EXPECT_THROW({
        try {
            JSON::Parse(invalidJson);
        } catch (const JSONParseException& e) {
            EXPECT_STREQ("JSON Parse Error: Unterminated string", e.what());
            throw;
        }
    }, JSONParseException);
}

TEST(JSONExeptionTest, InvalidBooleanException) {
    std::string invalidJson = "tru";
    EXPECT_THROW({
        try {
            JSON::Parse(invalidJson);
        } catch (const JSONParseException& e) {
            EXPECT_STREQ("JSON Parse Error: Invalid boolean value", e.what());
            throw;
        }
    }, JSONParseException);
}

TEST(JSONExeptionTest, InvalidNullException) {
    std::string invalidJson = "nul";
    EXPECT_THROW({
        try {
            JSON::Parse(invalidJson);
        } catch (const JSONParseException& e) {
            EXPECT_STREQ("JSON Parse Error: Invalid null value", e.what());
            throw;
        }
    }, JSONParseException);
}

TEST(JSONExeptionTest, MissingColonException) {
    std::string invalidJson = "{\"key\" \"value\"}";
    EXPECT_THROW({
        try {
            JSON::Parse(invalidJson);
        } catch (const JSONParseException& e) {
            EXPECT_STREQ("JSON Parse Error: Expected ':' after key", e.what());
            throw;
        }
    }, JSONParseException);
}

TEST(JSONExeptionTest, ValidJson) {
    std::string validJson = R"({
        "server": {
            "name": "SimpleServer",
            "port": 8080,
            "host": "192.168.1.1",
            "active": true
        },
        "database": {
            "type": "SQL",
            "user": "admin",
            "password": "secret"
        },
        "logging": {
            "level": "info",
            "file": "log.txt"
        }
    })";

    EXPECT_NO_THROW(JSON::Parse(validJson));
}


// Test with missing colon
TEST(JSONExeptionTest, MissingColon) {
    std::string invalidJson = R"({
        "server": {
            "name" "SimpleServer",  // Missing colon here
            "port": 8080,
            "host": "192.168.1.1",
            "active": true
        },
        "database": {
            "type": "SQL",
            "user": "admin",
            "password": "secret"
        },
        "logging": {
            "level": "info",
            "file": "log.txt"
        }
    })";

    EXPECT_THROW({
        try {
            JSON::Parse(invalidJson);
        } catch (const JSONParseException& e) {
            EXPECT_STREQ("JSON Parse Error: Expected ':' after key", e.what());
            throw;
        }
    }, JSONParseException);
}

// Test with incorrect boolean value
TEST(JSONExeptionTest, IncorrectBooleanValue) {
    std::string invalidJson = R"({
        "server": {
            "name": "SimpleServer",
            "port": 8080,
            "host": "192.168.1.1",
            "active": treu  // Misspelled boolean value
        },
        "database": {
            "type": "SQL",
            "user": "admin",
            "password": "secret"
        },
        "logging": {
            "level": "info",
            "file": "log.txt"
        }
    })";

    EXPECT_THROW({
        try {
            JSON::Parse(invalidJson);
        } catch (const JSONParseException& e) {
            EXPECT_STREQ("JSON Parse Error: Invalid boolean value", e.what());
            throw;
        }
    }, JSONParseException);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
