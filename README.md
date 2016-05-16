# ngrest-db

Small and easy in use ORM to work together with ngrest.


Highlights:
 - C++ structures are associated with DB tables
 - easy and intuitive syntax to perform most used db operations
 - code generator for maximum comfort and speed of development
 - use the meta-comments to provide additional, database-specific functionality (PK, FK, UNIQUE, etc..)
 - automatic mapping the developer-provided structures to database tables
 - easily integrated with ngrest services

Current stage: alpha, only basic features and only SQLite driver is implemented.

 
## Quick tour

**Example of C++ entity bound to DB table:**
```C++

// *table: users
struct User
{
    // be default all the fields are not null

    // *pk: true
    // *autoincrenent: true
    int id;

    std::string name;

    // *unique: true
    std::string email;
};
```

**Basic example of insering and querying the data from DB:**
```C++
#include <ngrest/db/SQLiteDb.h>
#include <ngrest/db/Table.h>

// ...

ngrest::SQLiteDb db("test.db"); // database connection
ngrest::Table<User> users(db); // object to work with users table

// exclude "id" from fields when inserting to make auto-increment working
users.setInsertFieldsInclusion({"id"}, ngrest::FieldsInclusion::Exclude);

// insert few users into 'users' table
// note: id is generated with AUTOINCREMENT
users << User {0, "John", "john@example.com"}
      << User {0, "James", "james@example.com"}
      << User {0, "Jane", "jane@example.com"};

// inserting the list using stream operators is possible too.
// also this is the prefered way to insert large amount of items
users << std::list<User>({
    {0, "Martin", "martin@example.com"},
    {0, "Marta", "marta@example.com"}
});

// select a user by id
users("id = ?", 1) >> user; // selects John

// std::ostream operators are generated too
std::cout << "User with id = 1 is: " << user << std::endl;

// select multiple users
std::list<User> userList;
users("name LIKE ?", "Ja%") >> userList; // selects James, Jane

```

**Advanced usage, some examples on how to use Table methods:**

```C++
// create table by schema if not exists
users.create();

// delete all data from the table
users.deleteAll();

// insert users using 'insert' method
users.insert({0, "Willy", "willy@example.com"});
users.insert({0, "Sally", "sally@example.com"});

// select all
const std::list<User>& resList0 = users.select();

// the count of query arguments is not limited (works with stream operators as well)
// selects Willy, Sally
const std::list<User>& resList2 = users.select("id > ? AND name LIKE ?", 2, "%lly");

// select only specified fields, other fields are has default initialization
const std::list<User>& resList6 = users.selectFields({"id", "name"}, "id = ?", 1);

// select one user
const User& resOne2 = users.selectOne("id = ?", 1);


// query using std::tuple
typedef std::tuple<int, std::string, std::string> UserInfo;

const std::list<UserInfo>& resRaw2 =
    users.selectTuple<UserInfo>({"id", "name", "email"}, "id > ?", 3);

for (const UserInfo& info : resRaw2) {
    int id;
    std::string name;
    std::string email;

    std::tie(id, name, email) = info;

    std::cout << id << " " << name << " " << email << std::endl;
}

// query one item using std::tuple

const UserInfo& resRaw3 = users.selectOneTuple<UserInfo>({"id", "name", "email"}, "id = ?", 2);

int id;
std::string name;
std::string email;

std::tie(id, name, email) = resRaw3;

std::cout << id << " " << name << " " << email << std::endl;

```

## TODO: 
 - installation using ngrest script like `ngrest install loentar/ngrest-db`
 - integrate into ngrest script as plugin to generate project files with ngrest-db support and other actions
 - implement MySQL driver
 - implement PostgreSQL driver
 - implement nullable, needs ngrest support
 - implement JOINs


