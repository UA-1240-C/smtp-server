#include <iostream>
#include <fstream>
#include <exception>

#include "MailDB/PgMailDB.h"

using namespace std;
using namespace ISXMailDB;

int main()
{
    Mail m = {"recipeint", "sender", "subject", "body"};
    std::cout << m << std::endl;
}