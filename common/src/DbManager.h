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

#ifndef NGREST_DBMANAGER_H
#define NGREST_DBMANAGER_H

#include <algorithm>
#include <unordered_map>

#include <ngrest/utils/Exception.h>
#include <ngrest/utils/Log.h>
#include <ngrest/db/Table.h>

namespace ngrest {
namespace detail {

template <int index>
inline void instantiateTables(Db& db, TableBase* tables[getEntityCount()])
{
    tables[index] = new Table<typename DataTypeWrapper<index>::type>(db);
    instantiateTables<index + 1>(db, tables);
}

template <>
inline void instantiateTables<getEntityCount()>(Db&, TableBase*[getEntityCount()])
{
}

}

template <class DbDriver>
class DbManager
{
public:
    template <typename... Params>
    DbManager(const Params... params):
        database(params...)
    {
        detail::instantiateTables<0>(database, tables);
    }

    virtual ~DbManager()
    {
        for (unsigned long i = 0; i < getEntityCount(); ++i)
            delete tables[i];
    }

    inline Db& db()
    {
        return database;
    }

    template <typename DataType>
    Table<DataType>& getTable()
    {
        return *static_cast<Table<DataType>*>(tables[getEntityIndex<DataType>()]);
    }

    TableBase* getTableByName(const std::string& tableName)
    {
        for (unsigned long i = 0; i < getEntityCount(); ++i) {
            const Entity& entity = tables[i]->getEntity();
            if (entity.getTableName() == tableName) {
                return tables[i];
            }
        }

        NGREST_THROW_ASSERT("No such table: " + tableName);
    }

    bool createAllTables(std::list<std::string>* createdTables = nullptr)
    {
        std::unordered_map<std::string, std::set<std::string>> tablesDeps;
        std::set<std::string> existing;
        std::set<std::string> entities;
        std::list<std::string> missing;
        std::list<std::string> toCreate;

        std::string name;
        Query query(database);
        query.prepare(database.getExistingTablesQuery());
        while (query.next()) {
            query.resultString(0, name);
            existing.insert(name);
        }

        for (unsigned long i = 0; i < getEntityCount(); ++i) {
            const Entity& entity = tables[i]->getEntity();
            entities.insert(entity.getTableName());
            for (const Field& field : entity.getFields()) {
                if (field.fk) {
                    tablesDeps[entity.getTableName()].insert(field.fk->entity.getTableName());
                }
            }
        }

        std::set_difference(entities.begin(), entities.end(),
                            existing.begin(), existing.end(),
                            std::inserter(missing, missing.begin()));

        bool changed;
        do {
            changed = false;
            for (auto it = missing.begin(); it != missing.end();) {
                const std::set<std::string>& deps = tablesDeps[*it];
                if (!deps.empty()) {
                    // check all required deps
                    bool found = false;
                    for (const std::string& dep : deps) {
                        found = (*it == dep) // fk to this table
                                || (std::find(toCreate.begin(), toCreate.end(), dep) != toCreate.end())
                                || (existing.find(dep) != existing.end());
                        if (!found)
                            break;
                    }
                    if (!found) {
                        ++it;
                        continue;
                    }
                }
                toCreate.push_back(*it);
                it = missing.erase(it);
                changed = true;
            }
        } while (changed);

        if (!missing.empty()) {
            std::string err = "Unable to create all tables. Some tables has unresolved dependencies: \n";
            for (const std::string& mis : missing) {
                err += " - " + mis + " =>";
                const std::set<std::string>& deps = tablesDeps[mis];
                for (const std::string& dep : deps) {
                    if (std::find(toCreate.begin(), toCreate.end(), dep) == toCreate.end() && existing.find(dep) == existing.end()) {
                        err += " " + dep;
                    }
                }
                err += "\n";
            }
            NGREST_THROW_ASSERT(err);
        }

        for (const std::string& tableName : toCreate) {
            LogDebug() << "Creating table: " << tableName;
            getTableByName(tableName)->create();
        }

        if (createdTables)
            *createdTables = toCreate;

        return !toCreate.empty();
    }

private:
    DbDriver database;
    TableBase* tables[getEntityCount()];
};

} // namespace ngrest

#endif // NGREST_DBMANAGER_H
