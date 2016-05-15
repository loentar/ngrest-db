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

#ifndef NGREST_DB_ENTITY_H
#define NGREST_DB_ENTITY_H

#include <list>
#include <string>

namespace ngrest {

struct Field;
struct Db;

//! parent class for code-generated entities
class Entity
{
public:
    virtual ~Entity() {}

    virtual const std::string& getName() const = 0;
    virtual const std::string& getTableName() const = 0;
    virtual const std::list<std::string>& getFieldsNames() const = 0;
    virtual const std::string& getFieldsNamesStr() const = 0;
    virtual const std::string& getFieldsArgs() const = 0;
    virtual const std::list<Field>& getFields() const = 0;
};

template <typename DataType>
const Entity& getEntityByDataType(); // implemented in codegenerated code

template <typename DataType>
constexpr unsigned long getEntityFieldsCount(); // implemented in codegenerated code

} // namespace ngrest


#endif // NGREST_DB_ENTITY_H
