#ifndef NGREST_DB_TEST_ENTITIES_H
#define NGREST_DB_TEST_ENTITIES_H

#include <string>
#include <iostream>
#include <ngrest/common/Nullable.h>

namespace ngrest {
namespace test {

enum TestEnum
{
    Val1,
    Val2,
    Val3
};

// *table: test1
struct Test1
{
    // *pk: true
    // *autoincrement: true
    int id;

    // *default: default value
    std::string defStr;
    // *default: true
    bool defB;
    // *default: 1.0
    double defD;
    // enum is stored as int
    // *default: 2
    TestEnum defE;


    std::string str;
    bool b;
    double d;
    TestEnum e;

    Nullable<std::string> nstr;
    Nullable<bool> nb;
    Nullable<double> nd;
    Nullable<TestEnum> ne;

    bool operator==(const Test1& other) const
    {
        return
            id == other.id &&
            defStr == other.defStr &&
            defB == other.defB &&
            defD == other.defD &&
            defE == other.defE &&
            str == other.str &&
            b == other.b &&
            d == other.d &&
            e == other.e &&
            nstr == other.nstr &&
            nb == other.nb &&
            nd == other.nd &&
            ne == other.ne;
    }
};

// *table: test2
struct Test2
{
    // *pk: true
    // *autoincrement: true
    int id;

    Nullable<int> nid;
};


} // namespace test
} // namespace ngrest

#endif // NGREST_DB_TEST_ENTITIES_H
