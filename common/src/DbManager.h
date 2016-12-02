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

#include <ngrest/utils/Exception.h>
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

private:
    DbDriver database;
    TableBase* tables[getEntityCount()];
};

} // namespace ngrest

#endif // NGREST_DBMANAGER_H
