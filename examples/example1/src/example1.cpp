#include <list>
#include <iostream>

#include <ngrest/utils/Log.h>

#ifdef HAS_SQLITE
#include <ngrest/db/SQLiteDb.h>
#endif
#ifdef HAS_MYSQL
#include <ngrest/db/MySqlDb.h>
#endif
#ifdef HAS_POSTGRES
#include <ngrest/db/PostgresDb.h>
#endif
#include <ngrest/db/Table.h>

#include "datatypes.h"


template <typename Item>
std::ostream& operator<<(std::ostream& out, const std::list<Item>& items)
{
    for (const Item& item : items)
        out << item << std::endl;

    return out;
}


namespace ngrest {
namespace example {



void example1(Db& db)
{
    ngrest::Table<User> users(db);

    users.create();

    users.deleteAll(); // delete all data from the table

    // exclude "id" from the fields when inserting to perform auto-increment
    // enabled by default
//    users.setInsertFieldsInclusion({"id"}, ngrest::FieldsInclusion::Exclude);

    users.insert({0, "John", "john@example.com"}); // id is ignored upon insertion

    int id = users.lastInsertId();
//    LogInfo() << "Last insert id: " << id;

    User user = {0, "James", "james@example.com"};
    users.insert(user);
    // users.insert(user, {"id", "registered"}, ngrest::FieldsInclusion::Exclude); // for example

    user.name = "Jane";
    user.email = "jane@example.com";
    users.insert(user);

    // for example
    std::tie(user.name, user.email) = std::tuple<std::string, std::string>("Jade", "jade@example.com");
    users.insert(user);


    // select all
    const std::list<User>& resList0 = users.select();
    NGREST_ASSERT(resList0.size() == 4, "Test failed");

    // select query
    const std::list<User>& resList1 = users.select("id = ?", id);
    NGREST_ASSERT(resList1.size() == 1, "Test failed");

    // select query and 2 parameters
    const std::list<User>& resList2 = users.select("id >= ? AND name LIKE ?", 0, "Ja%");
    NGREST_ASSERT(resList2.size() == 3, "Test failed");

    // select query and IN statement
//    const std::list<User>& resList3 = users.select("id IN ?", std::list<int>{1, 2, 3});
    // select query and IN statement
//    const std::list<User>& resList4 = users.select("id IN ? AND name LIKE ?",
//            std::list<int>{1, 2, 3}, "%Ja%");
    // select all items, desired fields
    const std::list<User>& resList5 = users.selectFields({"id", "name"}, "");
    NGREST_ASSERT(resList5.size() == 4, "Test failed");

    // select desired fields with query
    const std::list<User>& resList6 = users.selectFields({"id", "name"}, "id = ?", id);
    NGREST_ASSERT(resList6.size() == 1, "Test failed");


    const User& resOne2 = users.selectOne("id = ?", id);
    NGREST_ASSERT(resOne2.id == id, "Test failed");



//    const std::tuple<int, std::string, std::string>& resRaw1 =
//        users.selectOneTuple<std::tuple<int, std::string, std::string>>(
//        {"id", "name", "email"});


    // select just one field: all ids
    const std::list<int>& resRawIds = users.selectTuple<int>({"id"});
    NGREST_ASSERT(resRawIds.size() == 4, "Test failed");


    typedef std::tuple<int, std::string, std::string> UserInfo;

//    const std::list<std::tuple<int, std::string, std::string>>& resRaw1 =
//            users.selectTuple<std::tuple<int, std::string, std::string>>(
//            {"id", "name", "email"}, "id = ?", 2);

    // equals to resRaw1
    const std::list<UserInfo>& resRaw2 =
            users.selectTuple<UserInfo>({"id", "name", "email"}, "id = ?", id + 1);
    NGREST_ASSERT(resRaw2.size() == 1, "Test failed");

    for (const UserInfo& info : resRaw2) {
        int id;
        std::string name;
        std::string email;

        std::tie(id, name, email) = info;

//        std::cout << id << name << email << std::endl;
    }


    const UserInfo& resRaw3 = users.selectOneTuple<UserInfo>(
                {"id", "name", "email"}, "id = ?", id + 1);

    int resId;
    std::string name;
    std::string email;

    std::tie(resId, name, email) = resRaw3;

//    std::cout << resId << " " << name << " " << email << std::endl;


    // stream

    User user1 = {0, "Martin", "martin@example.com"};
    User user2 = {0, "Marta", "marta@example.com"};

    // insert
    users << user1 << user2;

    // select one item
    users("id = ?", id) >> user;

    // select multiple items
    std::list<User> userList1;
    users("id <= ?", id + 2) >> userList1;
    NGREST_ASSERT(userList1.size() == 3, "Test failed");

    // select all
    std::list<User> userList2;
    users() >> userList2;
    NGREST_ASSERT(userList2.size() == 6, "Test failed");


//    std::cout
//    << resList0  << std::endl
//    << "--------------------------"  << std::endl
//    << resList1  << std::endl
//    << "--------------------------"  << std::endl
//    << resList2  << std::endl
//    << "--------------------------"  << std::endl
//    << resList5  << std::endl
//    << "--------------------------"  << std::endl
//    << resList6  << std::endl
//    << "--------------------------"  << std::endl
//    << resOne2   << std::endl
//    << "--------------------------"  << std::endl
//    << user << std::endl
//    << "--------------------------"  << std::endl
//    << userList1   << std::endl
//    << "--------------------------"  << std::endl
//    << userList2   << std::endl;

}

}
}

int main()
{
    try {
#ifdef HAS_SQLITE
        ngrest::SQLiteDb sqliteDb("test.db");
        ngrest::example::example1(sqliteDb);
        ngrest::LogInfo() << "SQLite driver test passed";
#endif
#ifdef HAS_MYSQL
        // must have db and user created using statements:
        //   CREATE DATABASE test_ngrestdb;
        //   CREATE USER 'ngrestdb'@'localhost' IDENTIFIED BY 'ngrestdb';
        //   GRANT ALL PRIVILEGES ON test_ngrestdb TO 'ngrestdb'@'localhost' WITH GRANT OPTION;

        ngrest::MySqlDb mysqlDb({"test_ngrestdb", "ngrestdb", "ngrestdb"});
        ngrest::example::example1(mysqlDb);
        ngrest::LogInfo() << "MySQL driver test passed";
#endif
#ifdef HAS_POSTGRES
        // must have db and user created using statements:
        //   CREATE DATABASE test_ngrestdb;
        //   CREATE USER ngrestdb WITH password 'ngrestdb';
        //   GRANT ALL PRIVILEGES ON DATABASE test_ngrestdb TO ngrestdb;

        ngrest::PostgresDb postgresDb({"test_ngrestdb", "ngrestdb", "ngrestdb"});
        ngrest::example::example1(postgresDb);
        ngrest::LogInfo() << "Postgres driver test passed";
#endif
    } NGREST_CATCH_ALL
}
