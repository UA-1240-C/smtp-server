#pragma once

#include <gtest/gtest.h>
#include <chrono>
#include <thread>

#include "MailDB/PgMailDB.h"
#include "MailDB/MailException.h"
#include "MailDB/PgManager.h"

#include "Utils.h"

using namespace ISXMailDB;

const char*  CACHE_CONNECTION_STRING = "dbname=testmaildb user=postgres password=password hostaddr=127.0.0.1 port=5432";

class CacheTest : public testing::Test
{
protected:
  static void SetUpTestSuite() 
  {
    if (!s_connection.is_open())
    {
      FAIL() << "couldn't establish connection with test db\n";
    }
    pqxx::work tx(s_connection);

    ASSERT_NO_FATAL_FAILURE(
      ExecuteQueryFromFile(tx, DB_TABLE_CREATION_FILE)
    );

    s_database_manager = std::make_unique<PgManager>(CACHE_CONNECTION_STRING, "testhost", true);
  }

  static void TearDownTestSuite() 
  {
    pqxx::work tx(s_connection);

    tx.exec("DROP TABLE hosts, users, \"emailMessages\", \"mailBodies\"");
    tx.commit();

    s_connection.close();
  }

  CacheTest() : pg(*s_database_manager) {}
  ~CacheTest() = default;


  void TearDown() override
  {
    pqxx::work tx(s_connection);

    tx.exec("TRUNCATE TABLE \"emailMessages\" RESTART IDENTITY CASCADE;");
    tx.exec("TRUNCATE TABLE \"mailBodies\" RESTART IDENTITY CASCADE;");
    tx.exec("TRUNCATE TABLE users RESTART IDENTITY CASCADE;");

    tx.commit();
  }

  PgMailDB pg;
  static std::unique_ptr<PgManager> s_database_manager;
  static pqxx::connection s_connection;  
};

pqxx::connection CacheTest::s_connection{CACHE_CONNECTION_STRING};
std::unique_ptr<PgManager> CacheTest::s_database_manager = nullptr;

TEST_F(CacheTest, DefaultTest)
{
    pg.SignUp("user1", "pass1");
    pg.Login("user1", "pass1");

    pg.InsertEmail("user1", "sub1", "body1", {});
    pg.InsertEmail("user1", "sub2", "body2", {});

    std::this_thread::sleep_for(3*s_database_manager->get_writer_timeout());

    auto result = pg.RetrieveEmails();
    EXPECT_EQ(2, result.size());
}
