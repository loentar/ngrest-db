#include <list>
#include <iostream>

#include <ngrest/utils/Log.h>
#include <ngrest/utils/console.h>
#include <ngrest/utils/tocstring.h>

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

#include "test1.h"


template <typename Item>
std::ostream& operator<<(std::ostream& out, const std::list<Item>& items)
{
    for (const Item& item : items)
        out << item << std::endl;

    return out;
}

template <typename T>
std::ostream& operator<<(std::ostream& out, const ngrest::Nullable<T>& val)
{
    if (val.isNull()) {
        out << "null";
    } else {
        out << *val;
    }
    return out;
}

namespace ngrest {
namespace test {

void printCmp(const Test1& a, const Test1& b)
{
    std::cout << ((a.id == b.id)         ? colorDefault : colorTextMagenta) << "id     " << a.id       << " / " << b.id     << colorDefault << std::endl;
    std::cout << ((a.defStr == b.defStr) ? colorDefault : colorTextMagenta) << "defStr " << a.defStr   << " / " << b.defStr << colorDefault << std::endl;
    std::cout << ((a.defB == b.defB)     ? colorDefault : colorTextMagenta) << "defB   " << a.defB     << " / " << b.defB   << colorDefault << std::endl;
    std::cout << ((a.defD == b.defD)     ? colorDefault : colorTextMagenta) << "defD   " << a.defD     << " / " << b.defD   << colorDefault << std::endl;
    std::cout << ((a.defE == b.defE)     ? colorDefault : colorTextMagenta) << "defE   " << a.defE     << " / " << b.defE   << colorDefault << std::endl;
    std::cout << ((a.str == b.str)       ? colorDefault : colorTextMagenta) << "str    " << a.str      << " / " << b.str    << colorDefault << std::endl;
    std::cout << ((a.b == b.b)           ? colorDefault : colorTextMagenta) << "b      " << a.b        << " / " << b.b      << colorDefault << std::endl;
    std::cout << ((a.d == b.d)           ? colorDefault : colorTextMagenta) << "d      " << a.d        << " / " << b.d      << colorDefault << std::endl;
    std::cout << ((a.e == b.e)           ? colorDefault : colorTextMagenta) << "e      " << a.e        << " / " << b.e      << colorDefault << std::endl;
    std::cout << ((a.nstr == b.nstr)     ? colorDefault : colorTextMagenta) << "nstr   " << a.nstr     << " / " << b.nstr   << colorDefault << std::endl;
    std::cout << ((a.nb == b.nb)         ? colorDefault : colorTextMagenta) << "nb     " << a.nb       << " / " << b.nb     << colorDefault << std::endl;
    std::cout << ((a.nd == b.nd)         ? colorDefault : colorTextMagenta) << "nd     " << a.nd       << " / " << b.nd     << colorDefault << std::endl;
    std::cout << ((a.ne == b.ne)         ? colorDefault : colorTextMagenta) << "ne     " << a.ne       << " / " << b.ne     << colorDefault << std::endl;
}

void test1(Db& db, const std::string& driverName)
{
    std::cout << "------------ testing " + driverName + " driver -----------------\n";
    int passed = 0;
    int failed = 0;
    auto expect = [&](bool ex, const char* text) {
        std::cout << "Testing " << text;
        if (ex) {
            std::cout << colorBright << colorTextGreen << logResultSuccess << colorDefault << std::endl;
            ++passed;
        } else {
            std::cout << colorBright << colorTextRed << logResultFailed << colorDefault << std::endl;
            ++failed;
        }
    };

    ngrest::Table<Test1> tableTest1(db);

    tableTest1.create();

    tableTest1.deleteAll(); // delete all data from the table

    Test1 test1 = {0, "test", false, 2.2, Val2, "str", true, 3.3, Val2};
    tableTest1.insert(test1); // id is ignored upon insertion

    int id1 = tableTest1.lastInsertId();
    expect(id1 > 0, "last insert id > 0");
    test1.id = id1;

    const std::list<Test1>& res1 = tableTest1.select();
    expect(res1.size() == 1, "one item inserted and selected");
    expect(res1.front().id == id1, "last insert id match");

    const Test1& test1Res = res1.front();
    bool eq = (test1Res == test1);
    expect(eq, "inserted item = selected item 11");

    if (!eq)
        printCmp(test1, test1Res);


    Test1 test2 = {0, "test 2", true, 3.2, Val1, "str 2", false, 3.1, Val1, std::string("asdasd 2"), true, 12.1, Val3};
    tableTest1.insert(test2);

    int id2 = tableTest1.lastInsertId();
    expect(id2 > 0, "last insert id > 0");
    test2.id = id2;


    Test1 test3 = {0, "test 3", false, 3.3, Val3, "str 3", true, 3.3, Val3, std::string("asdasd 3"), false, 12.3, Val1};
    tableTest1.insert(test3);

    int id3 = tableTest1.lastInsertId();
    expect(id3 > 0, "last insert id > 0");
    test3.id = id3;


    const std::list<Test1>& res3 = tableTest1.select();
    expect(res3.size() == 3, "3 items inserted and selected");
    expect(res3.front().id == id1, "last insert id match");
    expect(res3.back().id == id3, "last insert id match");


    const Test1& test31Res = res3.front();
    eq = (test31Res == test1);
    expect(eq, "inserted item = selected item 31");
    if (!eq)
        printCmp(test1, test31Res);


    const Test1& test32Res = *(++res3.begin());
    eq = (test32Res == test2);
    expect(eq, "inserted item = selected item 32");
    if (!eq)
        printCmp(test2, test32Res);


    const Test1& test33Res = res3.back();
    eq = (test33Res == test3);
    expect(eq, "inserted item = selected item 33");
    if (!eq)
        printCmp(test3, test33Res);


    if (failed == 0) {
        std::cout << "  all " << passed << " tests passed\n";
        std::cout << "---------- " + driverName + " driver test PASSED ---------------\n\n";
    } else {
        std::cerr << colorBright << colorTextRed << "  failed " << failed << " tests of " << (passed + failed) << "\n";
        std::cerr << "---------- " + driverName + " driver test FAILED ---------------\n\n" << colorDefault;
    }
}

}
}

int main()
{
    try {
#ifdef HAS_SQLITE
        ngrest::SQLiteDb sqliteDb("test.db");
        ngrest::test::test1(sqliteDb, "SQLite");
#endif

#ifdef HAS_MYSQL
        // must have db and test1 created using statements:
        //   CREATE DATABASE test_ngrestdb;
        //   CREATE USER 'ngrestdb'@'localhost' IDENTIFIED BY 'ngrestdb';
        //   GRANT ALL PRIVILEGES ON test_ngrestdb TO 'ngrestdb'@'localhost' WITH GRANT OPTION;

        ngrest::MySqlDb mysqlDb({"test_ngrestdb", "ngrestdb", "ngrestdb"});
        ngrest::test::test1(mysqlDb, "MySQL");
#endif

#ifdef HAS_POSTGRES
        // must have db and test1 created using statements:
        //   CREATE DATABASE test_ngrestdb;
        //   CREATE USER ngrestdb WITH password 'ngrestdb';
        //   GRANT ALL PRIVILEGES ON DATABASE test_ngrestdb TO ngrestdb;

        ngrest::PostgresDb postgresDb({"test_ngrestdb", "ngrestdb", "ngrestdb"});
        ngrest::test::test1(postgresDb, "PostgreSQL");
#endif
    } catch (const std::exception& exception) {
        ::ngrest::LogError() << "Test failed: \n" << exception.what();
        return 1;
    } catch (...) {
        ::ngrest::LogError() << "Test failed!";
        return 1;
    }
}
