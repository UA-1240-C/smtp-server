#pragma once

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

        m_database->Login("user1", "pass1");
        m_database->InsertEmail("user2", "subject1", "looooong body1");
        m_database->Login("user2", "pass2");
        m_database->InsertEmail("user3", "subject2", "small body2");
        m_database->Login("user3", "pass3");
        m_database->InsertEmail("user1", "subject3", "medium body3");
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
        s_con_pool = std::make_shared<ConnectionPool<pqxx::connection>>(3, CONNECTION_STRING2, 
        [] (const std::string& connection_str)
        { 
            return std::make_shared<pqxx::connection>(connection_str);
        }
  );
        m_database = std::make_unique<ISXMailDB::PgMailDB>("host", *s_con_pool);
    }

  protected:
      inline static std::unique_ptr<ISXMailDB::PgMailDB> m_database;
      inline static std::shared_ptr<ConnectionPool<pqxx::connection>> s_con_pool;
};


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
    m_database->Login("user3", "pass3");
    EXPECT_NO_THROW(m_database->InsertEmail("user2", "subjectsubject", "body"));
    m_database->Login("user2", "pass2");
    EXPECT_EQ(2, m_database->RetrieveEmails().size());
}

TEST_F(DatabaseFixture, Insert_Email_With_Unexisting_Sender_Receiver_Test)
{   
    m_database->Logout();
    EXPECT_ANY_THROW(m_database->InsertEmail("user2", "subjectsubject", "body"));
    m_database->Login("user3", "pass3");
    EXPECT_ANY_THROW(m_database->InsertEmail("user54235", "subjectsubject", "body"));
    EXPECT_ANY_THROW(m_database->InsertEmail("", "subjectsubject", "body"));
}

TEST_F(DatabaseFixture, Insert_Blank_Email_Test)
{
    m_database->Login("user1", "pass1");
    EXPECT_NO_THROW(m_database->InsertEmail("user2", "", ""));
    m_database->Login("user2", "pass2");
    EXPECT_EQ(2, m_database->RetrieveEmails().size());
}

TEST_F(DatabaseFixture, Delete_Email_With_Existing_User_Test)
{
    EXPECT_NO_THROW(m_database->DeleteEmail("user1"));
    m_database->Login("user1", "pass1");
    EXPECT_EQ(0, m_database->RetrieveEmails().size());
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
