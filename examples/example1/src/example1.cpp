#include <list>
#include <iostream>

#include <ngrest/utils/Log.h>

#ifdef HAS_SQLITE
#include <ngrest/db/SQLiteDb.h>
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



void example1()
{
#ifdef HAS_SQLITE
    ngrest::SQLiteDb db("test.db");
#elif HAS_MYSQL
    ngrest::MySqlDb db("localhost", 5600, "test");
#else
#error no SQL driver available
#endif
//    ngrest::Db* db; /// FIXME!
    ngrest::Table<User> users(db);

    users.create();

    users.deleteAll(); // delete all data from the table

    // exclude "id" from the fields when inserting to perform auto-increment
    users.setInsertFieldsInclusion({"id"}, ngrest::FieldsInclusion::Exclude);

    users.insert({0, "John", "john@example.com", 0}); // id is ignored upon insertion

    User user = {0, "James", "james@example.com", 1};
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
    // select query
    const std::list<User>& resList1 = users.select("id = ?", 1);
    // select query and 2 parameters
    const std::list<User>& resList2 = users.select("id = ? AND name LIKE ?", 1, "%Ja%");
    // select query and IN statement
//    const std::list<User>& resList3 = users.select("id IN ?", std::list<int>{1, 2, 3});
    // select query and IN statement
//    const std::list<User>& resList4 = users.select("id IN ? AND name LIKE ?",
//            std::list<int>{1, 2, 3}, "%Ja%");
    // select all items, desired fields
    const std::list<User>& resList5 = users.selectFields({"id", "name"}, "");
    // select desired fields with query
    const std::list<User>& resList6 = users.selectFields({"id", "name"}, "id = ?", 1);


    const User& resOne2 = users.selectOne("id = ?", 1);



//    const std::tuple<int, std::string, std::string>& resRaw1 =
//        users.selectOneTuple<std::tuple<int, std::string, std::string>>(
//        {"id", "name", "email"});


    typedef std::tuple<int, std::string, std::string> UserInfo;

//    const std::list<std::tuple<int, std::string, std::string>>& resRaw1 =
//            users.selectTuple<std::tuple<int, std::string, std::string>>(
//            {"id", "name", "email"}, "id = ?", 2);

    // equals to resRaw1
    const std::list<UserInfo>& resRaw2 =
            users.selectTuple<UserInfo>({"id", "name", "email"}, "id = ?", 2);

    for (const UserInfo& info : resRaw2) {
        int id;
        std::string name;
        std::string email;

        std::tie(id, name, email) = info;

        std::cout << id << name << email << std::endl;
    }


    const UserInfo& resRaw3 = users.selectOneTuple<UserInfo>(
                {"id", "name", "email"}, "id = ?", 2);

    int id;
    std::string name;
    std::string email;

    std::tie(id, name, email) = resRaw3;

    std::cout << id << " " << name << " " << email << std::endl;


    // stream

    User user1 = {0, "Martin", "martin@example.com", 1};
    User user2 = {0, "Marta", "marta@example.com", 1};

    // insert
    users << user1 << user2;

    // select one item
    users("id = ?", 1) >> user;

    // select multiple items
    std::list<User> userList1;
    users("id <= ?", 3) >> userList1;

    // select all
    std::list<User> userList2;
    users() >> userList2;


    std::cout
    << resList0  << std::endl
    << "--------------------------"  << std::endl
    << resList1  << std::endl
    << "--------------------------"  << std::endl
    << resList2  << std::endl
    << "--------------------------"  << std::endl
    << resList5  << std::endl
    << "--------------------------"  << std::endl
    << resList6  << std::endl
    << "--------------------------"  << std::endl
    << resOne2   << std::endl
    << "--------------------------"  << std::endl
    << user << std::endl
    << "--------------------------"  << std::endl
    << userList1   << std::endl
    << "--------------------------"  << std::endl
    << userList2   << std::endl;

}

}
}

int main()
{
    try {
    ngrest::example::example1();
    } NGREST_CATCH_ALL
}
