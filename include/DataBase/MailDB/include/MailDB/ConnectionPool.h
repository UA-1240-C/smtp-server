#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <cstdint>

#include "MailException.h"

namespace ISXMailDB
{

template <typename ConnectionType>
class ConnectionPool {
public:
    using ConnCreationFunction = std::function<std::shared_ptr<ConnectionType>(const std::string&)>;

    ConnectionPool(uint16_t pool_size, const std::string& connection_str,
                   ConnCreationFunction create_connection)
        : m_connection_string(connection_str)
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

    std::shared_ptr<ConnectionType> Acquire() 
    {
        std::unique_lock<std::mutex> lock(m_pool_mutex);

        // Wait until a connection becomes available
        m_cv.wait(lock, [this]() { return !m_pool.empty(); });

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

private:
    std::queue<std::shared_ptr<pqxx::connection>> m_pool;  //< The connection pool
    std::mutex m_pool_mutex;                               //< Mutex for thread safety
    std::condition_variable m_cv;                          //< Condition variable to manage waiting threads
    std::string m_connection_string;                       //< Connection string for PostgreSQL
    const uint16_t MAX_DATABASE_CONNECTIONS = 10;          //< Max number of database connections
};

}
