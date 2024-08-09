# How to install
1. Copy libs/MailDB to your project libs folder
2. You may need to download PostgreSQL https://www.postgresql.org/download/
3. Don't forget to add library to your CMakeLists.txt
# How to use
Use PgMailDB class to establish connection with database. PgMailDB inherits from interface IMailDB. Some methods can throw an error (see https://github.com/UA-1240-C/DataBase/blob/main/Documentation.md).  It may be necessary to wrap all your code after the Connection() method in try catch block in order to catch Internet connection loss. For multithreading, you need to use in each thread own instance of PgMailDB.
# Example
```C++
PgMailDB pg("host or server name");

try
{
  // use "postgresql://postgres.qotrdwfvknwbfrompcji:yUf73LWenSqd9Lt4@aws-0-eu-central-1.pooler.supabase.com:6543/postgres?sslmode=require"
  // to connect to remote db
  pg.Connect(connection_string);
}
catch (const MailException& e)
{
  cerr << "connection failed\n";
}

```
# Database
ER diagram https://lucid.app/lucidchart/95544ac2-f894-4635-9e8f-5bbcb3618d7f/edit?view_items=4xwLouG-UXPv&invitationId=inv_6d37cbf3-607e-45b2-84d3-daaf9d50173c
