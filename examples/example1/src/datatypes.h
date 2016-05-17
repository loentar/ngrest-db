#ifndef NGREST_DB_EXAMPLE_ENTITIES_H
#define NGREST_DB_EXAMPLE_ENTITIES_H

#include <string>

namespace ngrest {
namespace example {

// *table: users
struct User
{
    // *pk: true
    // *autoincrement: true
    int id;

    std::string name;

    // *unique: true
    std::string email;

    // default: CURRENT_TIMESTAMP
    long registered;
};

// *table: groups
struct Group
{
    // *pk: true
    // *autoincrement: true
    int id;

    std::string name;
};

// *table: users_to_groups
struct UserToGroup
{
    // *pk: true
    // *autoincrement: true
    int id;

    // *fk: users id
    // *onUpdate: cascade
    // *onDelete: cascade
    int userId;

    // *fk: groups id
    // *key: usertogroup_group_fk
    // *onUpdate: cascade
    // *onDelete: cascade
    int groupId;
};

} // namespace example
} // namespace ngrest

#endif // NGREST_DB_EXAMPLE_ENTITIES_H
