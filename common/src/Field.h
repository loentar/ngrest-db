/*
 *  Copyright 2016 Utkin Dmitry <loentar@gmail.com>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/*
 *  This file is part of ngrest-db: http://github.com/loentar/ngrest-db
 */

#ifndef NGREST_DB_FIELD_H
#define NGREST_DB_FIELD_H

#include <string>

namespace ngrest {

class Entity;

struct ForeignKey
{
    std::string keyName;
    const Entity& entity;
    std::string fieldName;
    std::string onDelete;
    std::string onUpdate;
};

//! field description of code-generated entities
struct Field
{
    enum class DataType {
        Unknown,
        Bool,
        Int,
        BigInt,
        Float,
        String
    };

    DataType type;
    std::string dbType; // DBMS specific type, e.g. VARCHAR(64). can be empty
    std::string name;
    std::string defaultValue;
    bool notNull;
    bool isPK;
    bool isUnique;
    bool isAutoincrement;
    const ForeignKey* fk;
};

} // namespace ngrest


#endif // NGREST_DB_FIELD_H
