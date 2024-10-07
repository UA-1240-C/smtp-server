#pragma once

#include <pqxx/pqxx>
#include <string>
#include <iostream>
#include <fstream>
#include <gtest/gtest.h>


#include "MailDB/PgMailDB.h"
#include "MailDB/MailException.h"

const char*  DB_INSERT_DUMMY_DATA_FILE = "../../../../../include/DataBase/MailDB/test/db_insert_dummy_data.txt";
const char*  DB_TABLE_CREATION_FILE = "../../../../../include/DataBase/MailDB/test/db_table_creation.txt";

void ExecuteQueryFromFile(pqxx::work& tx, std::string filename)
{
   std::ifstream file(filename);
    if (!file.is_open()) 
    {
      FAIL() << "couldn't open a file " << filename << "\n"; 
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sql_commands = buffer.str();

    tx.exec(sql_commands);
    tx.commit();
}

std::string HashPassword(const std::string& password)
{
    std::string hashed_password(crypto_pwhash_STRBYTES, '\0');

    if (crypto_pwhash_str(hashed_password.data(),
        password.c_str(),
        password.size(),
        crypto_pwhash_OPSLIMIT_INTERACTIVE,
        crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0)
    {
        throw ISXMailDB::MailException("Password hashing failed");
    }

    return hashed_password;
}

bool VerifyPassword(const std::string& password, const std::string& hashed_password)
{
    return crypto_pwhash_str_verify(hashed_password.c_str(), password.c_str(),
                                    password.size()) == 0;
}