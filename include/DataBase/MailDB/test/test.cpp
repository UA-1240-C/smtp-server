#include <gtest/gtest.h>
#include <iostream>

#include "DatabaseFixture.h"
#include "PgMailDBTest.h"
#include "ConnPoolPqxxTest.h"

#include "MailDB/ConnectionPool.h"

int main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}