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

#include <set>
#include <algorithm>

#include <mysql/mysql.h>

#include <ngrest/utils/Exception.h>
#include <ngrest/utils/File.h>
#include <ngrest/utils/Log.h>
#include <ngrest/utils/tostring.h>
#include <ngrest/utils/fromcstring.h>
#include <ngrest/utils/stringutils.h>
#include <ngrest/utils/MemPool.h>
#include <ngrest/db/QueryImpl.h>
#include <ngrest/db/Entity.h>
#include "MySqlDb.h"

namespace ngrest {

class MySqlInitializer
{
public:
    MySqlInitializer():
        recursion(0)
    {
    }

    void init()
    {
        if (++recursion == 1)
            NGREST_ASSERT(mysql_library_init(0, nullptr, nullptr) == 0, "Failed to initialize MySQL!");
    }

    void deinit()
    {
        if (--recursion == 0)
            mysql_library_end();
    }

    static MySqlInitializer& inst()
    {
        static MySqlInitializer instance;
        return instance;
    }

private:
    int recursion;
};


class MySqlDbImpl
{
public:
    MySqlDbSettings settings;
    const std::set<std::string> supportedDmlStmt = {"INSERT", "REPLACE", "UPDATE", "DELETE", "SELECT", "SET"};

    MySqlDbImpl(const MySqlDbSettings& settings_):
        settings(settings_)
    {
    }
};

class MySqlQueryImpl: public QueryImpl
{
private:
    MySqlDb* db;
    MYSQL conn;
    int paramCount = 0;
    MYSQL_BIND* bindParams = nullptr;
    MYSQL_STMT* stmt = nullptr;
    int fieldsCount = 0;
    bool isConnected = false;
    bool doExecPrepared = true;
    bool hasResult = false;
    MYSQL_BIND* result = nullptr;
    MemPool pool;
    MemPool poolResult;

public:
    MySqlQueryImpl(MySqlDb* db_):
        db(db_)
    {
        mysql_init(&conn);
        MYSQL* pResult = mysql_real_connect(&conn, db->impl->settings.host.c_str(),
                                            db->impl->settings.login.c_str(),
                                            db->impl->settings.password.c_str(),
                                            db->impl->settings.db.c_str(),
                                            db->impl->settings.port, nullptr, 0);

        NGREST_ASSERT(pResult, std::string("Failed to connect to db: ") + mysql_error(&conn));

        isConnected = true;

        int result = mysql_set_character_set(&conn, "UTF8");
        NGREST_ASSERT(result == 0, std::string("error setting encoding: ") + mysql_error(&conn));
    }

    ~MySqlQueryImpl()
    {
        if (isConnected)
        {
            reset();
            mysql_close(&conn);
            isConnected = false;
        }
    }

    void reset() override
    {
        if (stmt)
        {
            mysql_stmt_close(stmt);
            stmt = nullptr;
        }
        fieldsCount = 0;
        bindParams = nullptr; // don't free(), it's in mempool
        result = nullptr;
        pool.reset();
        poolResult.reset();
    }

    void prepare(const std::string& query) override
    {
        NGREST_ASSERT(isConnected, "Not Initialized");
        NGREST_ASSERT(!stmt, "Already prepared. Use reset() to finalize query.");

        // workaround: MySQL does not support prepared statements for DML other than:
        //  INSERT, REPLACE, UPDATE, DELETE, SELECT, SET

        std::string::size_type start = query.find_first_not_of(" \n\r\t");
        NGREST_ASSERT(start != std::string::npos, "Empty query");
        std::string::size_type end = query.find_first_of(" \n\r\t", start);
        std::string dml = query.substr(start, (end != std::string::npos) ? (end - start) : end);

        std::transform(dml.begin(), dml.end(), dml.begin(), ::toupper);

        if (db->impl->supportedDmlStmt.find(dml) == db->impl->supportedDmlStmt.end()) {
            // execute query without using of stmt
            // no parameters are supported in that mode
            doExecPrepared = false;
            hasResult = false;
            int status = mysql_real_query(&conn, query.c_str(), query.size());
            NGREST_ASSERT(status == 0, "error executing query #" + toString(status) + ": \n"
                          + std::string(mysql_error(&conn))
                          + "\nWhile building query: \n----------\n" + query + "\n----------\n");
            return;
        }


        stmt = mysql_stmt_init(&conn);
        NGREST_ASSERT(stmt, "Can't init STMT: " + std::string(mysql_error(&conn))
                      + "\nWhile building query: \n----------\n" + query + "\n----------\n");

        try
        {
            int status = mysql_stmt_prepare(stmt, query.c_str(), query.size());
            NGREST_ASSERT(status == 0, "Failed to prepare STMT: "
                          + std::string(mysql_stmt_error(stmt))
                          + "\nQuery was:\n----------\n" + query + "\n----------\n");

            paramCount = static_cast<int>(mysql_stmt_param_count(stmt));

            bindParams = reinterpret_cast<MYSQL_BIND*>(pool.grow(sizeof(MYSQL_BIND) * paramCount));
            memset(bindParams, 0, sizeof(MYSQL_BIND) * paramCount);

            doExecPrepared = true;
            hasResult = true;
        }
        catch (...)
        {
            reset();
            throw;
        }
    }

    template <typename T>
    void bindValue(MYSQL_BIND* bind, T data)
    {
        bind->is_null_value = 0;
        bind->is_null = &bind->is_null_value;
        bind->buffer = pool.grow(sizeof(T));
        *reinterpret_cast<T*>(bind->buffer) = data;
        bind->buffer_length = sizeof(sizeof(T));
        bind->length = &bind->buffer_length;
    }

    void bindNull(int arg) override
    {
        NGREST_ASSERT(arg < paramCount, "Invalid arg number: " + toString(arg) + " of " + toString(paramCount));
        MYSQL_BIND* bind = &bindParams[arg];
        bind->buffer_type = MYSQL_TYPE_NULL;
        bind->is_null_value = 1;
        bind->is_null = &bind->is_null_value;
    }

    void bindBool(int arg, bool value) override
    {
        NGREST_ASSERT(arg < paramCount, "Invalid arg number: " + toString(arg) + " of " + toString(paramCount));
        MYSQL_BIND* bind = &bindParams[arg];
        bind->buffer_type = MYSQL_TYPE_TINY;
        bindValue(bind, static_cast<my_bool>(value));
    }

    void bindInt(int arg, int value) override
    {
        NGREST_ASSERT(arg < paramCount, "Invalid arg number: " + toString(arg) + " of " + toString(paramCount));
        MYSQL_BIND* bind = &bindParams[arg];
        bind->buffer_type = MYSQL_TYPE_LONG;
        bindValue(bind, value);
    }

    void bindBigInt(int arg, int64_t value) override
    {
        NGREST_ASSERT(arg < paramCount, "Invalid arg number: " + toString(arg) + " of " + toString(paramCount));
        MYSQL_BIND* bind = &bindParams[arg];
        bind->buffer_type = MYSQL_TYPE_LONGLONG;
        bindValue(bind, value);
    }

    void bindFloat(int arg, double value) override
    {
        NGREST_ASSERT(arg < paramCount, "Invalid arg number: " + toString(arg) + " of " + toString(paramCount));
        MYSQL_BIND* bind = &bindParams[arg];
        bind->buffer_type = MYSQL_TYPE_DOUBLE;
        bindValue(bind, value);
    }

    void bindString(int arg, const std::string& value) override
    {
        NGREST_ASSERT(arg < paramCount, "Invalid arg number: " + toString(arg) + " of " + toString(paramCount));
        MYSQL_BIND* bind = &bindParams[arg];
        bind->buffer_type = MYSQL_TYPE_STRING;
        bind->is_null_value = 0;
        bind->is_null = &bind->is_null_value;
        std::string::size_type size = value.size();
        bind->buffer = pool.putData(value.data(), size);
        bind->buffer_length = size;
        bind->length = &bind->buffer_length;
    }

    bool next() override
    {
        if (!stmt || !hasResult) // no results for exec
            return false;

        NGREST_ASSERT(stmt, "No statement prepared. Use prepare() before calling next().");

        if (doExecPrepared) {
            NGREST_ASSERT(mysql_stmt_bind_param(stmt, bindParams) == 0,
                          "Failed to bind params: \n" + std::string(mysql_stmt_error(stmt)));

            int status = mysql_stmt_execute(stmt);
            NGREST_ASSERT(status == 0, "Error #" + toString(status) + " executing query: \n"
                          + std::string(mysql_stmt_error(stmt)));

            // fetch result from server
            my_bool updateMaxLength = 1;
            mysql_stmt_attr_set(stmt, STMT_ATTR_UPDATE_MAX_LENGTH, &updateMaxLength);

            NGREST_ASSERT(!mysql_stmt_store_result(stmt), "Error storing result: \n" + std::string(mysql_stmt_error(stmt)));

            if (!mysql_stmt_num_rows(stmt)) {
                // no result
                hasResult = false;
                return false;
            }

            fieldsCount = static_cast<int>(mysql_stmt_field_count(stmt));

            result = reinterpret_cast<MYSQL_BIND*>(pool.grow(sizeof(MYSQL_BIND) * fieldsCount));
            memset(result, 0, sizeof(MYSQL_BIND) * fieldsCount);

            prepareResult();

            doExecPrepared = false;
        }

        int res = mysql_stmt_fetch(stmt);
        NGREST_ASSERT(res == 0 || res == MYSQL_NO_DATA, "Error fetching result: \n" + std::string(mysql_stmt_error(stmt)));

        return res == 0;
    }

    enum_field_types getNearestType(enum_field_types type)
    {
        switch (type) {
        // probably bool
        case MYSQL_TYPE_TINY:
        case MYSQL_TYPE_BIT:
            return MYSQL_TYPE_TINY;

            // int
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_DATE:
        case MYSQL_TYPE_TIME:
        case MYSQL_TYPE_YEAR:
        case MYSQL_TYPE_ENUM:
        case MYSQL_TYPE_LONG:
            return MYSQL_TYPE_LONG;

            // int64
        case MYSQL_TYPE_LONGLONG:
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_DATETIME:
        case MYSQL_TYPE_TIMESTAMP:
        case MYSQL_TYPE_NEWDATE:
            return MYSQL_TYPE_LONGLONG;

            // double
        case MYSQL_TYPE_DECIMAL:
        case MYSQL_TYPE_FLOAT:
        case MYSQL_TYPE_DOUBLE:
        case MYSQL_TYPE_NEWDECIMAL:
            return MYSQL_TYPE_DOUBLE;

            // fallback to string
        default:
            return MYSQL_TYPE_STRING;
        }
    }

    void prepareResult()
    {
        MYSQL_RES* meta = mysql_stmt_result_metadata(stmt);
        hasResult = !!meta;
        if (!meta) // INSERT, DELETE, etc...
            return;

        try {
            uint64_t totalMaxLen = 0;
            for (int field = 0; field < fieldsCount; ++field)
                totalMaxLen += meta->fields[field].max_length;

            poolResult.reset();
            poolResult.reserve(totalMaxLen);

            for (int field = 0; field < fieldsCount; ++field)
            {
                // using result provided type, maybe need to convert it for actual result*(column) later
                // this function is called before result*(column) so we don't know actual types here

                // make mysql cast type to nearest type of ours supported
                result[field].buffer_type = getNearestType(meta->fields[field].type);
                result[field].is_null = &result[field].is_null_value;

                unsigned long maxLength = meta->fields[field].max_length;
                if (maxLength)
                {
                    void* data = poolResult.grow(maxLength);
                    memset(data, 0, maxLength);
                    result[field].buffer = data;
                    result[field].buffer_length = maxLength;
                }
            }

            NGREST_ASSERT(!mysql_stmt_bind_result(stmt, result), "Can't bind result: \n"
                          + std::string(mysql_stmt_error(stmt)));
        } catch(...) {
            mysql_free_result(meta);
            throw;
        }

        mysql_free_result(meta);
    }

    MYSQL_BIND& fetchColumn(int column)
    {
        NGREST_ASSERT(column < fieldsCount, "Invalid column number: " + toString(column) + " of " + toString(fieldsCount));

        NGREST_ASSERT(!mysql_stmt_fetch_column(stmt, &result[column], column, 0),
                      "Failed to fetch column: " + std::string(mysql_stmt_error(stmt)));

        return result[column];
    }

    bool resultIsNull(int column) override
    {
        NGREST_ASSERT(column < fieldsCount, "Invalid column number: " + toString(column) + " of " + toString(fieldsCount));

        return result[column].is_null_value != 0;
    }

    template <typename T>
    T castResult(MYSQL_BIND& res)
    {
        NGREST_ASSERT_NULL(res.buffer);
        NGREST_ASSERT_NULL(res.length);
        NGREST_ASSERT(sizeof(T) == *res.length, "Size of type and buffer does not match");
        return *reinterpret_cast<T*>(res.buffer);
    }

    template <typename T>
    T resultNum(MYSQL_BIND& res)
    {
        switch (res.buffer_type) {
        case MYSQL_TYPE_TINY:
            return static_cast<T>(castResult<char>(res));

        case MYSQL_TYPE_LONG:
            return static_cast<T>(castResult<int>(res));

        case MYSQL_TYPE_LONGLONG:
            return static_cast<T>(castResult<int64_t>(res));

        case MYSQL_TYPE_DOUBLE:
            return static_cast<T>(castResult<double>(res));

        case MYSQL_TYPE_STRING: {
            NGREST_ASSERT_NULL(res.buffer);
            T ret = 0;
            fromCString(static_cast<const char*>(res.buffer), ret);
            return ret;
        }

        default:
            NGREST_THROW_ASSERT("Unexpected buffer type!");
        }
    }


    bool resultBool(int column) override
    {
        MYSQL_BIND& res = fetchColumn(column);

        switch (res.buffer_type) {
        case MYSQL_TYPE_TINY:
            return castResult<char>(res) != 0;

        case MYSQL_TYPE_LONG:
            return castResult<int>(res) != 0;

        case MYSQL_TYPE_LONGLONG:
            return castResult<int64_t>(res) != 0;

        case MYSQL_TYPE_DOUBLE:
            return castResult<double>(res) != 0;

        case MYSQL_TYPE_STRING: {
            NGREST_ASSERT_NULL(res.buffer);
            const char* val = reinterpret_cast<const char*>(res.buffer);
            return *val == 't' || *val == 'T' || *val == '1';
        }

        default:
            NGREST_THROW_ASSERT("Unexpected buffer type!");
        }

    }

    int resultInt(int column) override
    {
        return resultNum<int>(fetchColumn(column));
    }

    int64_t resultBigInt(int column) override
    {
        return resultNum<int64_t>(fetchColumn(column));
    }

    double resultFloat(int column) override
    {
        return resultNum<double>(fetchColumn(column));
    }

    void resultString(int column, std::string& value) override
    {
        MYSQL_BIND& res = fetchColumn(column);

        bool ok = false;
        switch (res.buffer_type) {
        case MYSQL_TYPE_TINY:
            toString(castResult<char>(res) & 0xff, value, &ok);
            break;

        case MYSQL_TYPE_LONG:
            toString(castResult<int>(res), value, &ok);
            break;

        case MYSQL_TYPE_LONGLONG:
            toString(castResult<int64_t>(res), value, &ok);
            break;

        case MYSQL_TYPE_DOUBLE:
            toString(castResult<double>(res), value, &ok);
            break;

        case MYSQL_TYPE_STRING:
            NGREST_ASSERT_NULL(res.buffer);
            NGREST_ASSERT_NULL(res.length);
            value.assign(reinterpret_cast<const char*>(res.buffer), *res.length);
            break;

        default:
            NGREST_THROW_ASSERT("Unexpected buffer type!");
        }
    }

    int64_t lastInsertId() override
    {
        NGREST_ASSERT(isConnected, "Not Initialized");
        return mysql_insert_id(&conn);
    }

};


MySqlDb::MySqlDb(const MySqlDbSettings& settings):
    impl(new MySqlDbImpl(settings))
{
    MySqlInitializer::inst().init();
}

MySqlDb::~MySqlDb()
{
    delete impl;
    MySqlInitializer::inst().deinit();
}

QueryImpl* MySqlDb::newQuery()
{
    return new MySqlQueryImpl(this);
}

std::string MySqlDb::getCreateTableQuery(const Entity& entity) const
{
    std::string fieldsStr;

    for (const Field& field : entity.getFields()) {
        if (!fieldsStr.empty())
            fieldsStr += ", ";
        fieldsStr += field.name + " ";
        fieldsStr += (!field.dbType.empty() ? field.dbType : getTypeName(field.type)) + " ";
        if (field.isPK)
            fieldsStr += "PRIMARY KEY ";
        if (field.isAutoincrement)
            fieldsStr += "AUTO_INCREMENT ";
        if (field.isUnique)
            fieldsStr += "UNIQUE ";
        if (field.notNull)
            fieldsStr += "NOT NULL ";
        if (!field.defaultValue.empty())
            fieldsStr += "DEFAULT " + field.defaultValue;
    }

    return "CREATE TABLE IF NOT EXISTS " + entity.getTableName() + " (" + fieldsStr + ")";
}

const std::string& MySqlDb::getTypeName(Field::DataType type) const
{
    static const int size = 6;
    static const std::string types[size] = {
        "VOID",
        "TINYINT",
        "INTEGER",
        "BIGINT",
        "DOUBLE",
        "VARCHAR(256)"
    };

    const int pos = static_cast<int>(type);
    return (pos > size || pos <= 0) ? types[0] : types[pos];
}

} // namespace ngrest
