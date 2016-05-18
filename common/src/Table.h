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

#ifndef NGREST_TABLE_H
#define NGREST_TABLE_H

#include <string>
#include <list>
#include <set>
#include <bitset>
#include <tuple>

#include <ngrest/utils/Exception.h>
#include <ngrest/db/Db.h>
#include <ngrest/db/Field.h>

#include "Query.h"

// codegenerated file
#include <tableEntities.h>

namespace ngrest {

class Db;

enum class FieldsInclusion
{
    NotSet,
    Include,
    Exclude
};

template <typename DataType>
class Table
{
public:
    typedef std::bitset<getEntityFieldsCount<DataType>()> FieldsSet;

    class ResultStreamer
    {
    public:
        ResultStreamer(QueryImpl* query_):
            query(query_)
        {
        }

        ResultStreamer& operator>>(DataType& item)
        {
            NGREST_ASSERT(query.next(), "Error executing query: no more rows");
            readDataFromQuery(query, item);
            return *this;
        }

        ResultStreamer& operator>>(std::list<DataType>& result)
        {
            DataType data;
            while (query.next()) {
                readDataFromQuery(query, data);
                result.push_back(data);
            }

            return *this;
        }

    private:
        Query query;
    };

public:
    Table(Db& db_, bool ignoreAutoincFieldsOnInsert = true):
        db(db_),
        entity(getEntityByDataType<DataType>())
    {
        if (ignoreAutoincFieldsOnInsert) {
            // find any autoincrement fields and ignore it
            for (const Field& field : entity.getFields()) {
                if (field.isAutoincrement) {
                    insertFields.insert(field.name);
                }
            }
            if (!insertFields.empty())
                insertInclusion = FieldsInclusion::Exclude;
        }
    }

    void create()
    {
        Query(db).query(db.getCreateTableQuery(entity));
    }

    void setInsertFieldsInclusion(const std::set<std::string>& fields, FieldsInclusion inclusion)
    {
        insertFields = fields;
        insertInclusion = inclusion;
    }

    void resetInsertFieldsInclusion()
    {
        insertInclusion = FieldsInclusion::NotSet;
    }

    Table<DataType>& insert(const DataType& item)
    {
        if (insertInclusion == FieldsInclusion::NotSet) {
            Query query(db);
            query.prepare("INSERT INTO " + entity.getTableName() + "(" + entity.getFieldsNamesStr() + ") "
                        + "VALUES" + entity.getFieldsArgs());
            bindDataToQuery(query, item);
            query.next();
            return *this;
        } else {
            return insert(item, insertFields, insertInclusion);
        }
    }

    Table<DataType>& insert(const DataType& item, const std::set<std::string>& fields, FieldsInclusion inclusion)
    {
        Query query(db);

        FieldsSet includedFields;
        std::string fieldsStr;
        std::string queryArgs;

        buildFieldQueryData(fields, inclusion, fieldsStr, includedFields, &queryArgs);

        query.prepare("INSERT INTO " + entity.getTableName() + "(" + fieldsStr + ") "
                    + "VALUES(" + queryArgs + ")");
        bindDataToQuery(query, item, includedFields);
        query.next();

        return *this;
    }

    Table<DataType>& insert(const std::list<DataType>& items) {
        if (insertInclusion == FieldsInclusion::NotSet) {
            Query query(db);
            query.prepare("INSERT INTO " + entity.getTableName() + "(" + entity.getFieldsNamesStr() + ") "
                        + "VALUES" + entity.getFieldsArgs());
            for (const DataType& item : items) {
                bindDataToQuery(query, item);
                query.next();
            }
            return *this;
        } else {
            return insert(items, insertFields, insertInclusion);
        }
    }


    Table<DataType>& insert(const std::list<DataType>& items, const std::set<std::string>& fields,
                FieldsInclusion inclusion = FieldsInclusion::Include)
    {
        Query query(db);

        FieldsSet includedFields;
        std::string fieldsStr;
        std::string queryArgs;

        buildFieldQueryData(fields, inclusion, fieldsStr, includedFields, &queryArgs);

        query.prepare("INSERT INTO " + entity.getTableName() + "(" + fieldsStr + ") "
                    + "VALUES(" + queryArgs + ")");

        for (const DataType& item : items) {
            bindDataToQuery(query, item, includedFields);
            query.next();
        }

        return *this;
    }

    Table<DataType>& operator<<(const DataType& item)
    {
        return insert(item);
    }

    Table<DataType>& operator<<(const std::list<DataType>& items)
    {
        return insert(items);
    }

    int64_t lastInsertId()
    {
        return Query(db).lastInsertId();
    }

    // select

    std::list<DataType> select()
    {
        std::list<DataType> result;
        Query query(db);
        query.prepare("SELECT " + entity.getFieldsNamesStr() + " FROM " + entity.getTableName());
        DataType data;
        while (query.next()) {
            readDataFromQuery(query, data);
            result.push_back(data);
        }

        return result;
    }

    template <typename... Params>
    std::list<DataType> select(const std::string& where, const Params... params)
    {
        Query query(db);
        query.prepare("SELECT " + entity.getFieldsNamesStr() + " FROM " + entity.getTableName() + " WHERE " + where);
        query.bindAll(params...);

        std::list<DataType> result;
        DataType data;
        while (query.next()) {
            readDataFromQuery(query, data);
            result.push_back(data);
        }

        return result;
    }

    template <typename... Params>
    std::list<DataType> selectFields(const std::set<std::string>& fields, FieldsInclusion inclusion,
                                     const std::string& where, const Params... params)
    {
        Query query(db);

        FieldsSet includedFields;
        std::string fieldsStr;
        buildFieldQueryData(fields, inclusion, fieldsStr, includedFields);

        std::string queryStr = "SELECT " + fieldsStr + " FROM " + entity.getTableName();
        if (!where.empty())
            queryStr += " WHERE " + where;
        query.prepare(queryStr);
        query.bindAll(params...);

        std::list<DataType> result;
        DataType data;
        while (query.next()) {
            readDataFromQuery(query, data, includedFields);
            result.push_back(data);
        }

        return result;
    }

    // shorthand version
    template <typename... Params>
    std::list<DataType> selectFields(const std::set<std::string>& fields, const std::string& where, const Params... params)
    {
        return selectFields(fields, FieldsInclusion::Include, where, params...);
    }

    // select one item

    template <typename... Params>
    DataType selectOne(const std::string& where, const Params... params)
    {
        Query query(db);

        query.prepare("SELECT " + entity.getFieldsNamesStr() + " FROM " + entity.getTableName()
                      + " WHERE " + where + " LIMIT 1");
        query.bindAll(params...);
        NGREST_ASSERT(query.next(), "Error executing query: no more rows");

        DataType result;
        readDataFromQuery(query, result);

        return result;
    }


    // select typle

    template<typename Tuple, typename... Params>
    std::list<Tuple> selectTuple(const std::list<std::string>& rowNames,
                                 const std::string& where, const Params... params)
    {
        Query query(db);
        std::string queryStr = "SELECT " + join(rowNames) + " FROM " + entity.getTableName();
        if (!where.empty())
            queryStr += " WHERE " + where;

        query.prepare(queryStr);
        query.bindAll(params...);

        std::list<Tuple> result;
        Tuple data;
        while (query.next()) {
            query.resultAll(data);
            result.push_back(data);
        }

        return result;
    }

    template<typename Tuple>
    std::list<Tuple> selectTuple(const std::list<std::string>& rowNames)
    {
        return selectTuple<Tuple>(rowNames, std::string());
    }

    // select one

    template<typename Tuple, typename... Params>
    Tuple selectOneTuple(const std::list<std::string>& rowNames,
            const std::string& where, const Params... params)
    {
        Query query(db);

        query.prepare("SELECT " + join(rowNames) + " FROM " + entity.getTableName() + " WHERE " + where + " LIMIT 1");
        query.bindAll(params...);

        NGREST_ASSERT(query.next(), "Error executing query: no more rows");
        Tuple result;
        query.resultAll(result);

        return result;
    }

    template <typename... Params>
    ResultStreamer operator()(const std::string& where, const Params... params)
    {
        Query query(db);
        query.prepare("SELECT " + entity.getFieldsNamesStr() + " FROM " + entity.getTableName() + " WHERE " + where);
        query.bindAll(params...);
        return ResultStreamer(query.take());
    }

    ResultStreamer operator()()
    {
        Query query(db);
        query.prepare("SELECT " + entity.getFieldsNamesStr() + " FROM " + entity.getTableName());
        return ResultStreamer(query.take());
    }

    // delete

    template <typename... Params>
    void deleteWhere(const std::string& where, const Params... params)
    {
        Query query(db);
        query.prepare("DELETE FROM " + entity.getTableName() + " WHERE " + where);
        query.bindAll(params...);
        query.next();
    }

    template <typename... Params>
    void deleteAll()
    {
        Query query(db);
        query.query("DELETE FROM " + entity.getTableName());
    }

private:
    void buildFieldQueryData(const std::set<std::string>& fields, FieldsInclusion inclusion,
                             std::string& fieldsStr, FieldsSet& includedFields, std::string* args = nullptr)
    {
        const std::list<std::string>& fieldsNames = entity.getFieldsNames();
        int tag = 0;
        for (const std::string& field : fieldsNames) {
            bool found = (fields.find(field) != fields.end());
            bool include = (found && (inclusion == FieldsInclusion::Include))
                    || (!found && (inclusion == FieldsInclusion::Exclude));
            if (include) {
                if (!fieldsStr.empty()) {
                    fieldsStr += ",";
                    if (args != nullptr)
                        *args += ",";
                }
                fieldsStr += field;
                if (args != nullptr)
                    *args += "?";
            }
            includedFields[tag] = include;

            ++tag;
        }
    }

    std::string join(const std::list<std::string>& strings)
    {
        std::string::size_type size = 0;
        for (const std::string& str : strings)
            size += str.size() + 1;

        std::string result;
        result.reserve(size);
        for (const std::string& str : strings) {
            if (!result.empty())
                result.append(",");
            result.append(str);
        }

        return result;
    }

private:
    Db& db;
    const Entity& entity;
    std::set<std::string> insertFields;
    FieldsInclusion insertInclusion = FieldsInclusion::NotSet;
};

} // namespace ngrest

#endif // NGREST_TABLE_H
