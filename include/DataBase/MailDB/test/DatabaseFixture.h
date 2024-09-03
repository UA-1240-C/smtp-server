#include <memory>

#include <gtest/gtest.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "MailDB/PgMailDB.h"
#include "MailDB/MailException.h"

const char*  CONNECTION_STRING2 = "dbname=testmaildb2 user=postgres password=password hostaddr=127.0.0.1 port=5432";

using namespace ISXMailDB;

class DatabaseFixture : public testing::Test
{
  public:
    DatabaseFixture() = default;
    ~DatabaseFixture() = default;

    virtual void SetUp() override
    {
        m_database->SignUp("user1", "pass1");
        m_database->SignUp("user2", "pass2");
        m_database->SignUp("user3", "pass3");

        m_database->InsertEmail("user1", "user2", "subject1", "looooong body1");
        m_database->InsertEmail("user2", "user3", "subject2", "small body2");
        m_database->InsertEmail("user3", "user1", "subject3", "medium body3");
    }

    virtual void TearDown() override
    {
        pqxx::connection conn(CONNECTION_STRING2);
        pqxx::work transaction(conn);
        transaction.exec(
            "TRUNCATE \"emailMessages\", \"mailBodies\", users RESTART IDENTITY"
        );
        transaction.commit();
    }
  
    static void SetUpTestCase()
    {
      m_database = std::make_unique<ISXMailDB::PgMailDB>("host");
      m_database->Connect(CONNECTION_STRING2);
    }

    static void TearDownTestCase()
    {
      m_database->Disconnect();
    }

  protected:
      inline static std::unique_ptr<ISXMailDB::PgMailDB> m_database;
};

TEST(ConnectionTestSuite, Connect_To_Local_Test)
{
    ISXMailDB::PgMailDB database("host");
    EXPECT_NO_THROW(database.Connect(CONNECTION_STRING2));

    database.Disconnect();
}

TEST(ConnectionTestSuite, Connect_To_Remote_Test)
{
    ISXMailDB::PgMailDB database("host");
    EXPECT_NO_THROW(database.Connect("postgresql://postgres.qotrdwfvknwbfrompcji:yUf73LWenSqd9Lt4@aws-0-eu-central-1.pooler.supabase.com:6543/postgres?sslmode=require"));

    database.Disconnect();
}

TEST(ConnectionTestSuite, Connect_To_Unexisting_DB_Test)
{
    ISXMailDB::PgMailDB database("host");
    EXPECT_ANY_THROW(database.Connect("dbname=db user=postgres password=1234 hostaddr=127.0.0.1 port=5432"));

    database.Disconnect();
}

TEST(ConnectionTestSuite, Connect_Several_Times_Test)
{
    ISXMailDB::PgMailDB database("host");
    EXPECT_NO_THROW(database.Connect("postgresql://postgres.qotrdwfvknwbfrompcji:yUf73LWenSqd9Lt4@aws-0-eu-central-1.pooler.supabase.com:6543/postgres?sslmode=require"));
    EXPECT_NO_THROW(database.Connect("postgresql://postgres.qotrdwfvknwbfrompcji:yUf73LWenSqd9Lt4@aws-0-eu-central-1.pooler.supabase.com:6543/postgres?sslmode=require"));
    EXPECT_NO_THROW(database.Connect("postgresql://postgres.qotrdwfvknwbfrompcji:yUf73LWenSqd9Lt4@aws-0-eu-central-1.pooler.supabase.com:6543/postgres?sslmode=require"));

    database.Disconnect();
}

TEST(ConnectionTestSuite, Disconnect_When_Connected_To_DB_Test)
{
    ISXMailDB::PgMailDB database("host");
    database.Connect("postgresql://postgres.qotrdwfvknwbfrompcji:yUf73LWenSqd9Lt4@aws-0-eu-central-1.pooler.supabase.com:6543/postgres?sslmode=require");
    EXPECT_TRUE(database.IsConnected());

    database.Disconnect();
    EXPECT_FALSE(database.IsConnected());
}

TEST(ConnectionTestSuite, Disconnect_When_Not_Connected_To_DB_Test)
{
    ISXMailDB::PgMailDB database("host");
    EXPECT_FALSE(database.IsConnected());

    database.Disconnect();
    EXPECT_FALSE(database.IsConnected());
}

TEST(ConnectionTestSuite, Disconnect_Several_Times_Test)
{
    ISXMailDB::PgMailDB database("host");
    EXPECT_FALSE(database.IsConnected());

    database.Disconnect();
    database.Disconnect();
    database.Disconnect();
    database.Disconnect();
    EXPECT_FALSE(database.IsConnected());

}

TEST_F(DatabaseFixture, Retrieve_Existing_User_Test)
{
   EXPECT_EQ(1, m_database->RetrieveUserInfo("user1").size());
}

TEST_F(DatabaseFixture, Retrieve_All_Users_Test)
{
   EXPECT_EQ(3, m_database->RetrieveUserInfo("").size());
}

TEST_F(DatabaseFixture, Retrieve_Unexisting_Users_Test)
{
   EXPECT_EQ(0, m_database->RetrieveUserInfo("user1234").size());
}

TEST_F(DatabaseFixture, Retrieve_Existing_Body_Content_Test)
{
   EXPECT_EQ(1, m_database->RetrieveEmailContentInfo("medium body3").size());
}

TEST_F(DatabaseFixture, Retrieve_Unexisting_Body_Content_Test)
{
    EXPECT_EQ(0, m_database->RetrieveEmailContentInfo("bodycontent").size());
}

TEST_F(DatabaseFixture, Retrieve_All_Body_Content_Test)
{
    EXPECT_EQ(3, m_database->RetrieveEmailContentInfo("").size());
}

TEST_F(DatabaseFixture, Insert_Email_With_Existing_Sender_Receiver_Test)
{
    EXPECT_NO_THROW(m_database->InsertEmail("user3", "user2", "subjectsubject", "body"));
    EXPECT_EQ(2, m_database->RetrieveEmails("user2").size());
}

TEST_F(DatabaseFixture, Insert_Email_With_Unexisting_Sender_Receiver_Test)
{
    EXPECT_ANY_THROW(m_database->InsertEmail("user3242", "user2", "subjectsubject", "body"));
    EXPECT_ANY_THROW(m_database->InsertEmail("user3", "user54235", "subjectsubject", "body"));
    EXPECT_ANY_THROW(m_database->InsertEmail("", "", "subjectsubject", "body"));
}

TEST_F(DatabaseFixture, Insert_Blank_Email_Test)
{
    EXPECT_NO_THROW(m_database->InsertEmail("user1", "user2", "", ""));
    EXPECT_EQ(2, m_database->RetrieveEmails("user2").size());
}

TEST_F(DatabaseFixture, Delete_Email_With_Existing_User_Test)
{
    EXPECT_NO_THROW(m_database->DeleteEmail("user1"));
    EXPECT_EQ(0, m_database->RetrieveEmails("user1").size());
}

TEST_F(DatabaseFixture, Delete_Email_With_Unexisting_User_Test)
{
    EXPECT_ANY_THROW(m_database->DeleteEmail("user3242"));
}

TEST_F(DatabaseFixture, Delete_Existing_User_Test)
{
    EXPECT_NO_THROW(m_database->DeleteUser("user1", "pass1"));
    EXPECT_EQ(0, m_database->RetrieveUserInfo("user1").size());
}

TEST_F(DatabaseFixture, Delete_Unexisting_User_Test)
{
    EXPECT_ANY_THROW(m_database->DeleteUser("user1213", "pass1"));
    EXPECT_ANY_THROW(m_database->DeleteUser("user1", "pass1dasdasd"));
}
