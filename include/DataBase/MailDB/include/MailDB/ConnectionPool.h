#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <cstdint>
#include <functional>

#include "MailException.h"

namespace ISXMailDB
{

template <typename ConnectionType>
class ConnectionPool {
public:
    using ConnCreationFunction = std::function<std::shared_ptr<ConnectionType>(const std::string&)>;

    ConnectionPool(uint16_t pool_size, const std::string& connection_str,
                   ConnCreationFunction create_connection)
        : m_connection_string(connection_str), m_timeout(std::chrono::seconds(20))
    {
        try
        {
            for (uint16_t i;i < std::min(pool_size, MAX_DATABASE_CONNECTIONS); ++i) 
            {
                m_pool.push(create_connection(m_connection_string));
            }
        }
        catch(const std::exception& e)
        {
            throw MailException(e.what());
        }
    }

    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;

    std::shared_ptr<ConnectionType> Acquire() 
    {
        std::unique_lock<std::mutex> lock(m_pool_mutex);

        // Wait until a connection becomes available, or until the timeout expires
        if (!m_cv.wait_for(lock, m_timeout, [this]() { return !m_pool.empty(); })) 
        {
            throw MailException("Timeout: No available connections after waiting.");
        }

        auto connection = m_pool.front();
        m_pool.pop();
        return connection;
    }

    void Release(std::shared_ptr<ConnectionType> connection)
    {
        std::unique_lock<std::mutex> lock(m_pool_mutex);
      
        m_pool.push(connection);
        lock.unlock();
        m_cv.notify_one();
    }

    void set_timeout(std::chrono::seconds timeout) { m_timeout = timeout;}

private:
    std::queue<std::shared_ptr<pqxx::connection>> m_pool;  ///< The connection pool
    std::mutex m_pool_mutex;                               ///< Mutex for thread safety
    std::condition_variable m_cv;                          ///< Condition variable to manage waiting threads
    std::string m_connection_string;                       ///< Connection string for PostgreSQL
    const uint16_t MAX_DATABASE_CONNECTIONS = 10;          ///< Max number of database connections
    std::chrono::seconds m_timeout; ///< Timeout to acquire connection
};

}
