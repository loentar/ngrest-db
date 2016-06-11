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

#include <postgresql/libpq-fe.h>

#include <ngrest/utils/Exception.h>
#include <ngrest/utils/File.h>
#include <ngrest/utils/Log.h>
#include <ngrest/utils/tostring.h>
#include <ngrest/utils/fromcstring.h>
#include <ngrest/utils/stringutils.h>
#include <ngrest/utils/MemPool.h>
#include <ngrest/db/QueryImpl.h>
#include <ngrest/db/Entity.h>
#include "PostgresDb.h"

namespace ngrest {

class PostgresDbImpl
{
public:
    PostgresDbSettings settings;

    PostgresDbImpl(const PostgresDbSettings& settings_):
        settings(settings_)
    {
    }
};

class PostgresQueryImpl: public QueryImpl
{
private:
    PostgresDb* db;
    PGconn* conn;
    int paramCount = 0;
    bool doingSelect = false;
    bool doExecPrepared = true;
    int* paramLengths = nullptr;
    char** paramValues = nullptr;
    MemPool pool;
    PGresult* result = nullptr;
    int currentRow = 0;
    int rowsCount = 0;
    int fieldsCount = 0;
    bool hasResult = false;

public:
    PostgresQueryImpl(PostgresDb* db_):
        db(db_)
    {
        conn = PQsetdbLogin(db->impl->settings.host.c_str(),
                            toString(db->impl->settings.port).c_str(), "", "",
                            db->impl->settings.db.c_str(),
                            db->impl->settings.login.c_str(),
                            db->impl->settings.password.c_str());

        NGREST_ASSERT(conn, std::string("Failed to connect to db: "));
        if (PQstatus(conn) != CONNECTION_OK) {
            const std::string& err = std::string("Failed to login: ") + PQerrorMessage(conn);
            PQfinish(conn);
            conn = nullptr;
            NGREST_THROW_ASSERT(err);
        }

        int result = PQsetClientEncoding(conn, "UTF8");
        NGREST_ASSERT(result == 0, std::string("error setting encoding: ") + PQerrorMessage(conn));
    }

    ~PostgresQueryImpl()
    {
        if (conn)
        {
            reset();
            PQfinish(conn);
        }
    }

    void reset() override
    {
        if (result)
        {
            PQclear(result);
            result = nullptr;
        }
        fieldsCount = 0;
        paramLengths = nullptr;
        paramValues = nullptr;
    }

    void prepare(const std::string& query) override
    {
        NGREST_ASSERT(conn, "Not Initialized");
        NGREST_ASSERT(!result, "Already prepared. Use reset() to finalize query.");

        // postgres only supports "$1, $2"... as query placeholders
        // we replace "?, ?" it to it. use "\\?" for escaping

        std::string fixedQuery = query;
        std::string strIndex;
        paramCount = 0;
        std::string::size_type pos = 0;
        while ((pos = fixedQuery.find("?", pos)) != std::string::npos)
        {
            if (pos > 0 && fixedQuery[pos - 1] == '\\') { // skip "\\?"
                ++pos;
                continue;
            }
            ++paramCount;
            toString(paramCount, strIndex);
            fixedQuery.replace(pos, 1, "$" + strIndex);
            pos += strIndex.size() + 1;
        }

        paramLengths = reinterpret_cast<int*>(pool.grow(sizeof(int) * paramCount));
        paramValues = reinterpret_cast<char**>(pool.grow(sizeof(const char*) * paramCount));

        PGresult* res = PQprepare(conn, "", fixedQuery.c_str(), paramCount, nullptr);

        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            const std::string& error = "Failed to prepare statement: " + std::string(PQerrorMessage(conn));
            PQclear(res);
            NGREST_THROW_ASSERT(error);
        }
        PQclear(res);

        currentRow = 0;
        doExecPrepared = true;


        std::string::size_type start = query.find_first_not_of(" \n\r\t");
        NGREST_ASSERT(start != std::string::npos, "Empty query");
        std::string::size_type end = query.find_first_of(" \n\r\t", start);
        std::string dml = query.substr(start, (end != std::string::npos) ? (end - start) : end);

        std::transform(dml.begin(), dml.end(), dml.begin(), ::toupper);

        doingSelect = (dml == "SELECT");
    }

    template <typename T>
    void bindValue(int arg, T value)
    {
        NGREST_ASSERT(arg < paramCount, "Invalid arg number: " + toString(arg) + " of " + toString(paramCount));

        char* buffer = pool.grow(NGREST_NUM_TO_STR_BUFF_SIZE);

        bool isOk = toCString(value, buffer, NGREST_NUM_TO_STR_BUFF_SIZE);
        NGREST_ASSERT(isOk, "Failed to convert a number to string");
        const size_t buffLen = strlen(buffer) + 1;

        pool.shrinkLastChunk(NGREST_NUM_TO_STR_BUFF_SIZE - buffLen);

        paramValues[arg] = buffer;
        paramLengths[arg] = static_cast<int>(buffLen);
    }

    void bindNull(int arg) override
    {
        NGREST_ASSERT(arg < paramCount, "Invalid arg number: " + toString(arg) + " of " + toString(paramCount));

        paramValues[arg] = nullptr;
        paramLengths[arg] = 0;
    }

    void bindBool(int arg, bool value) override
    {
        bindValue(arg, static_cast<char>(value ? 1 : 0));
    }

    void bindInt(int arg, int value) override
    {
        bindValue(arg, value);
    }

    void bindBigInt(int arg, int64_t value) override
    {
        bindValue(arg, value);
    }

    void bindFloat(int arg, double value) override
    {
        bindValue(arg, value);
    }

    void bindString(int arg, const std::string& value) override
    {
        NGREST_ASSERT(arg < paramCount, "Invalid arg number: " + toString(arg) + " of " + toString(paramCount));

        const size_t length = value.size() + 1;
        paramValues[arg] = pool.putCString(value.c_str(), length);
        paramLengths[arg] = static_cast<int>(length);
    }

    bool next() override
    {
        NGREST_ASSERT(conn, "Not initialized.");

        if (doExecPrepared || !doingSelect) {
            result = PQexecPrepared(conn, "", paramCount, paramValues, paramLengths, nullptr, 0);

            NGREST_ASSERT(result, "Error executing query: \n" + std::string(PQerrorMessage(conn)));

            ExecStatusType status = PQresultStatus(result);
            NGREST_ASSERT(status == PGRES_TUPLES_OK || status == PGRES_COMMAND_OK,
                          "Failed to execute prepared statement: " + std::string(PQerrorMessage(conn)));

            hasResult = (status == PGRES_TUPLES_OK);

            rowsCount = PQntuples(result);
            fieldsCount = PQnfields(result);

            doExecPrepared = false;

            return currentRow < rowsCount;
        }

        if ((currentRow + 1) >= rowsCount) // no more results
            return false;

        ++currentRow;

        return true;
    }

    bool resultIsNull(int column) override
    {
        NGREST_ASSERT(column < fieldsCount, "Invalid column number: " + toString(column) + " of " + toString(fieldsCount));

        return PQgetisnull(result, currentRow, column) != 0;
    }

    bool resultBool(int column) override
    {
        const char* value = PQgetvalue(result, currentRow, column);
        return !!value && (*value == 't' || *value == 'T' || *value == '1');
    }

    template <typename T>
    T getResult(int column)
    {
        const char* valueStr = PQgetvalue(result, currentRow, column);
        NGREST_ASSERT_NULL(valueStr);
        T value = 0;
        NGREST_ASSERT(fromCString(valueStr, value), "Failed to convert from string");
        return value;
    }

    int resultInt(int column) override
    {
        return getResult<int>(column);
    }

    int64_t resultBigInt(int column) override
    {
        return getResult<int64_t>(column);
    }

    double resultFloat(int column) override
    {
        return getResult<double>(column);
    }

    void resultString(int column, std::string& value) override
    {
        const char* valueStr = PQgetvalue(result, currentRow, column);
        NGREST_ASSERT_NULL(valueStr);
        int len = PQgetlength(result, currentRow, column);
        value.assign(valueStr, len);
    }

    int64_t lastInsertId() override
    {
        NGREST_ASSERT(conn, "Not Initialized");

        reset();
        query("SELECT LASTVAL()");
        return resultBigInt(0);
    }

};


PostgresDb::PostgresDb(const PostgresDbSettings& settings):
    impl(new PostgresDbImpl(settings))
{
}

PostgresDb::~PostgresDb()
{
    delete impl;
}

QueryImpl* PostgresDb::newQuery()
{
    return new PostgresQueryImpl(this);
}

std::string PostgresDb::getCreateTableQuery(const Entity& entity) const
{
    std::string fieldsStr;

    for (const Field& field : entity.getFields()) {
        if (!fieldsStr.empty())
            fieldsStr += ", ";
        fieldsStr += field.name + " ";

        if (field.dbType.empty()) {
            if (field.isAutoincrement) {
                fieldsStr += "SERIAL ";
            } else {
                fieldsStr += getTypeName(field.type) + " ";
            }
        } else {
            fieldsStr += field.dbType + " ";
        }
        if (field.isPK)
            fieldsStr += "PRIMARY KEY ";
        if (field.isUnique)
            fieldsStr += "UNIQUE ";
        if (field.notNull)
            fieldsStr += "NOT NULL ";
        if (!field.defaultValue.empty()) {
            if (field.type == Field::DataType::String) {
                fieldsStr += "DEFAULT '" + field.defaultValue + "'";
            } else {
                fieldsStr += "DEFAULT " + field.defaultValue;
            }
        }
    }

    return "CREATE TABLE IF NOT EXISTS " + entity.getTableName() + " (" + fieldsStr + ")";
}

const std::string& PostgresDb::getTypeName(Field::DataType type) const
{
    static const int size = static_cast<int>(Field::DataType::Last);
    static const std::string types[size] = {
        "VOID",
        "BOOL",
        "INTEGER",
        "BIGINT",
        "DOUBLE PRECISION",
        "INTEGER",
        "VARCHAR(256)"
    };

    const int pos = static_cast<int>(type);
    return (pos >= size || pos <= 0) ? types[0] : types[pos];
}

} // namespace ngrest
