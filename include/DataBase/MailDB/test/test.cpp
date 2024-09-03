#include <gtest/gtest.h>
#include <iostream>

#include "DatabaseFixture.h"
#include "PgMailDBTest.h"



const char*  CONNECTION_STRING = "dbname=testmaildb2 user=postgres password=password hostaddr=127.0.0.1 port=5432";





int main(int argc, char **argv)
{
 testing::InitGoogleTest(&argc, argv);
 return RUN_ALL_TESTS();
}