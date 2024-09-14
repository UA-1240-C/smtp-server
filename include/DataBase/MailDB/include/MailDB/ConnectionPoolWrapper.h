#pragma once

#include "ConnectionPool.h"

namespace ISXMailDB
{

template <typename ConnectionType>
class ConnectionPoolWrapper {
public:
    ConnectionPoolWrapper(ConnectionPool<ConnectionType>& pool)
        : m_connection_pool(pool), m_connection(pool.Acquire()) 
    {}

    ~ConnectionPoolWrapper() 
    {
        m_connection_pool.Release(m_connection);
    }

    ConnectionType& operator*() {
        return *m_connection;
    }

    ConnectionType* operator->() {
        return m_connection.get();
    }

private:
    ConnectionPool<ConnectionType>& m_connection_pool;        //< Reference to the connection pool
    std::shared_ptr<ConnectionType> m_connection;             //< The acquired connection
};

}