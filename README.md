# ngrest-db

Small and easy in use ORM to work together with ngrest.


Highlights:
 - mapping the developer-provided structures to database tables
 - easy and intuitive syntax to perform most used db operations
 - code generator for maximum comfort and speed of development
 - use the meta-comments to provide additional, database-specific functionality (PK, FK, UNIQUE, etc..)
 - easy to integrate with ngrest services

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

## Installation

Must have ngrest installed: https://github.com/loentar/ngrest.

No special install command needed. To add this package to your project type from project directory:

```
ngrest add loentar/ngrest-db sqlite
```

## Example on how to use ngrest-db with ngrest service

Create service:

```
ngrest create Notes
```

Add ngrest-db package
```
cd notes
ngrest add loentar/ngrest-db sqlite
```

Make your `notes/src/Notes.h` look like this:

```C++
#ifndef NOTES_H
#define NOTES_H

#include <list>
#include <unordered_map>
#include <ngrest/common/Service.h>

// *table: notes
struct Note
{
    // *pk: true
    // *autoincrement: true
    int id;

    // *unique: true
    std::string title;

    std::string text;
};

// *location: notes
class Notes: public ngrest::Service
{
public:
    //! add a note, returns inserted id
    // *method: POST
    int add(const std::string& title, const std::string& text);

    //! remove node by id
    // *method: DELETE
    void remove(int id);

    //! get note by id
    Note get(int id);

    //! get list of ids of all notes
    std::list<int> ids();

    //! get map of notes: id, title
    std::unordered_map<int, std::string> list();

    //! find notes containing title
    std::list<Note> find(const std::string& title);
};


#endif // NOTES_H
```


And `notes/src/Notes.cpp` like this:

```C++
#include <ngrest/db/SQLiteDb.h>
#include <ngrest/db/Table.h>

#include "Notes.h"

// singleton to access notes db
class NotesDb
{
public:
    static NotesDb& inst()
    {
        static NotesDb instance;
        return instance;
    }

    ngrest::Db& db()
    {
        return notesDb;
    }

    ngrest::Table<Note>& notes()
    {
        return notesTable;
    }

private:
    NotesDb():
        notesDb("notes.db"),
        notesTable(notesDb)
    {
        // perform DB initialization here
        notesTable.create(); // create table if not exist
    }

    ngrest::SQLiteDb notesDb;
    ngrest::Table<Note> notesTable;
};


int Notes::add(const std::string& title, const std::string& text)
{
    return NotesDb::inst().notes()
            .insert({0, title, text})
            .lastInsertId();
}

void Notes::remove(int id)
{
    NotesDb::inst().notes().deleteWhere("id = ?", id);
}

Note Notes::get(int id)
{
    return NotesDb::inst().notes().selectOne("id = ?", id);
}

std::list<int> Notes::ids()
{
    return NotesDb::inst().notes().selectTuple<int>({"id"});
}

std::unordered_map<int, std::string> Notes::list()
{
    // example of access to DB without using Table<>
    std::unordered_map<int, std::string> result;

    ngrest::Query query(NotesDb::inst().db());
    query.prepare("SELECT id, title FROM notes");

    while (query.next()) {
        // read id from result(0), title from result(1)
        query.resultString(1, result[query.resultInt(0)]);
    }
    return result;
}

std::list<Note> Notes::find(const std::string& title)
{
    return NotesDb::inst().notes().select("title LIKE ?", "%" + title + "%");
}
```

Start your project from it's directory:

```
ngrest
```

Open ngrest services tester http://localhost:9098/ngrest/service/Notes and try implemented operations: add, get, find, etc...

The DB will be stored in `.ngrest/local/build/notes.db`.


## TODO: 
 - implement MySQL driver
 - implement PostgreSQL driver
 - implement nullable, needs ngrest support
 - implement JOINs
