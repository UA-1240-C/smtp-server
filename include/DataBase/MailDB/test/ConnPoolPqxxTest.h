#pragma once

#include <gtest/gtest.h>
#include <pqxx/pqxx>

#include "MailDB/MailException.h"
#include "MailDB/ConnectionPool.h"

const char*  POOLDB_CONNECTION_STRING = "dbname=pooltestdb user=pooltest password=password hostaddr=127.0.0.1 port=5432";

using namespace ISXMailDB;

std::shared_ptr<pqxx::connection> CreateConnection(const std::string& connection_str)
{ 
    return std::make_shared<pqxx::connection>(connection_str);
}

class ConnPoolPqxxTest : public testing::Test
{
protected:
    static void SetUpTestSuite()
    { 
        try
        {
            auto con = pqxx::connection(POOLDB_CONNECTION_STRING);
            pqxx::work tx(con);

            tx.exec(R""""(CREATE TABLE example_table (
                            id SERIAL PRIMARY KEY,
                            name TEXT NOT NULL
                        );
                        )"""");
            tx.commit();
        }
        catch(const std::exception& e)
        {
            FAIL() << "Fail in SetUpTestSuite : " << e.what();
        }
    }

    static void TearDownTestSuite() 
    {
        auto con = pqxx::connection(POOLDB_CONNECTION_STRING);
        pqxx::work tx(con);

        tx.exec("DROP TABLE example_table");
        tx.commit();
    }

    void SetUp() override 
    { 
        try
        {   
            m_con_pool = std::make_shared<ConnectionPool<pqxx::connection>>(POOL_START_SIZE, POOLDB_CONNECTION_STRING, CreateConnection);
            auto con = m_con_pool->Acquire();
            pqxx::work tx(*con);

            tx.exec("INSERT INTO example_table (name) VALUES ('test_user')");
            tx.commit();

            m_con_pool->Release(con);
        }
        catch(const std::exception& e)
        {
            FAIL() << "Fail in SetUp : " << e.what();
        }
     }

    void TearDown() override
    {
        auto con = m_con_pool->Acquire();
        pqxx::work tx(*con);

        tx.exec("TRUNCATE TABLE example_table RESTART IDENTITY CASCADE;");

        m_con_pool->Release(con);
    }

    std::shared_ptr<ConnectionPool<pqxx::connection>> m_con_pool;
    const uint16_t POOL_START_SIZE = 3;
};

TEST_F(ConnPoolPqxxTest, TooManyConnectionsTest)
{
    EXPECT_THROW(
        std::make_shared<ConnectionPool<pqxx::connection>>(3, POOLDB_CONNECTION_STRING, CreateConnection)
        , std::exception
    );
}

TEST_F(ConnPoolPqxxTest, AcquireConnection) {
    auto conn = m_con_pool->Acquire();
    EXPECT_NE(conn, nullptr); 
    m_con_pool->Release(conn);
}

TEST_F(ConnPoolPqxxTest, ReleaseConnection) {
    auto conn = m_con_pool->Acquire();
    EXPECT_NE(conn, nullptr); 
    m_con_pool->Release(conn); 

    for (size_t i = 0; i < POOL_START_SIZE-1; i++)
    {
        auto c = m_con_pool->Acquire();
        m_con_pool->Release(c);
    } 
    auto acquiredConn = m_con_pool->Acquire(); // Acquire the same connection again
    EXPECT_EQ(acquiredConn, conn); 
}

TEST_F(ConnPoolPqxxTest, AcquireAndReleaseConnection) {
    for(size_t i = 0; i < POOL_START_SIZE*3+1; i++)
    {
        auto conn = m_con_pool->Acquire();
        ASSERT_NE(conn, nullptr); 
        pqxx::work tx(*conn);

        tx.exec(std::format("INSERT INTO example_table (name) VALUES ('test_user{}')", i));
        tx.commit();

        m_con_pool->Release(conn);
    }

    for(size_t i = 0; i < POOL_START_SIZE*3+1; i++)
    {
        auto conn = m_con_pool->Acquire();
        ASSERT_NE(conn, nullptr); 
        pqxx::work tx(*conn);

        auto res = tx.query_value<int>(std::format("SELECT 1 FROM example_table WHERE name ='test_user{}'", i));
        tx.commit();
        
        EXPECT_EQ(1, res);
        m_con_pool->Release(conn);
    }
}