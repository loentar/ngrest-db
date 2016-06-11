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

#include <sqlite3.h>

#include <ngrest/utils/Exception.h>
#include <ngrest/utils/File.h>
#include <ngrest/utils/Log.h>
#include <ngrest/utils/tostring.h>
#include <ngrest/db/QueryImpl.h>
#include <ngrest/db/Entity.h>
#include "SQLiteDb.h"

namespace ngrest {

class SQLiteDbImpl
{
public:
    sqlite3* conn = nullptr;
    std::string dbPath;
};

class SQLiteQueryImpl: public QueryImpl
{
private:
    SQLiteDb* db;
    sqlite3_stmt* result = nullptr;
    unsigned fieldsCount = 0;

public:
    SQLiteQueryImpl(SQLiteDb* db_):
        db(db_)
    {
    }

    ~SQLiteQueryImpl()
    {
        reset();
    }

    void reset() override
    {
        if (result)
        {
            sqlite3_finalize(result);
            result = nullptr;
            fieldsCount = 0;
        }
    }

    void prepare(const std::string& query) override
    {
        NGREST_ASSERT(db->impl->conn, "Not Initialized");
        NGREST_ASSERT(!result, "Already prepared. Use reset() to finalize query.");

        int res = sqlite3_prepare_v2(db->impl->conn, query.c_str(), query.size(), &result, nullptr);
        NGREST_ASSERT(res == SQLITE_OK, "error #" + toString(res) + ": "
                      + std::string(sqlite3_errmsg(db->impl->conn))
                      + "; db: \"" + db->impl->dbPath + "\""
                      + "\nWhile building query: \n----------\n" + query + "\n----------\n");
    }

    inline void assertBindRes(int res)
    {
        NGREST_ASSERT(res == SQLITE_OK, "error #" + toString(res) + ": "
                      + std::string(sqlite3_errmsg(db->impl->conn))
                      + "; db: \"" + db->impl->dbPath + "\""
                      + "\nWhile binding query:\n"
                      + "\n----------\n"
                      + std::string(sqlite3_sql(result))
                      + "\n----------\n");
    }

    void bindNull(int arg) override
    {
        assertBindRes(sqlite3_bind_null(result, arg + 1));
    }

    void bindBool(int arg, bool value) override
    {
        assertBindRes(sqlite3_bind_int(result, arg + 1, value ? 1 : 0));
    }

    void bindInt(int arg, int value) override
    {
        assertBindRes(sqlite3_bind_int(result, arg + 1, value));
    }

    void bindBigInt(int arg, int64_t value) override
    {
        assertBindRes(sqlite3_bind_int64(result, arg + 1, value));
    }

    void bindFloat(int arg, double value) override
    {
        assertBindRes(sqlite3_bind_double(result, arg + 1, value));
    }

    void bindString(int arg, const std::string& value) override
    {
        assertBindRes(sqlite3_bind_text(result, arg + 1, value.c_str(), value.size(), SQLITE_TRANSIENT));
    }

    bool next() override
    {
        NGREST_ASSERT(result, "No statement prepared. Use prepare() before calling next().");

        int status = sqlite3_step(result);
        if (status == SQLITE_ROW)
            return true;

        NGREST_ASSERT(status == SQLITE_DONE || status == SQLITE_OK,
                      "error #" + toString(status) + ": "
                      + std::string(sqlite3_errmsg(db->impl->conn))
                      + "; db: \"" + db->impl->dbPath + "\""
                      + "\nWhile executing query: \n----------\n"
                      + std::string(sqlite3_sql(result))
                      + "\n----------\n");
        return false; // no data
    }

    bool resultIsNull(int column) override
    {
        return sqlite3_column_type(result, column) == SQLITE_NULL;
    }

    bool resultBool(int column) override
    {
        return sqlite3_column_int(result, column) != 0;
    }

    int resultInt(int column) override
    {
        return sqlite3_column_int(result, column);
    }

    int64_t resultBigInt(int column) override
    {
        return sqlite3_column_int64(result, column);
    }

    double resultFloat(int column) override
    {
        return sqlite3_column_double(result, column);
    }

    void resultString(int column, std::string& value) override
    {
        const char* res = reinterpret_cast<const char*>(sqlite3_column_text(result, column));
        value = res ? res : "";
    }

    int64_t lastInsertId() override
    {
        NGREST_ASSERT(db->impl->conn, "Not Initialized");
        return sqlite3_last_insert_rowid(db->impl->conn);
    }

};



SQLiteDb::SQLiteDb(const std::string& dbPath, const SQLiteDbSettings& settings):
    impl(new SQLiteDbImpl())
{
    NGREST_ASSERT(!impl->conn, "Already connected");
    if (settings.enableSharedCache)
        sqlite3_enable_shared_cache(true);

    if (!File(dbPath).isFile())
        LogDebug() << "Database file does not exists: \"" << dbPath << "\"";

    // open db
    int result = sqlite3_open(dbPath.c_str(), &impl->conn);
    NGREST_ASSERT(result == SQLITE_OK, "Failed to open database: " + dbPath);

    if (settings.enableFK) {
        sqlite3_stmt* stmt = nullptr;

        result = sqlite3_prepare_v2(impl->conn, "PRAGMA foreign_keys = ON;", -1, &stmt, nullptr);
        NGREST_ASSERT(result == SQLITE_OK, sqlite3_errmsg(impl->conn));

        try {
            NGREST_ASSERT(sqlite3_step(stmt) == SQLITE_DONE, "Failed to enable foreign keys: "
                          + std::string(sqlite3_errmsg(impl->conn)));
        } catch (...) {
            sqlite3_finalize(stmt);
            throw;
        }

        NGREST_ASSERT(sqlite3_finalize(stmt) == SQLITE_OK, sqlite3_errmsg(impl->conn));
    }

    impl->dbPath = dbPath;
}

SQLiteDb::~SQLiteDb()
{
    if (impl->conn) {
        // free all prepared ops
        sqlite3_stmt* stmt;
        while ((stmt = sqlite3_next_stmt(impl->conn, 0)))
            sqlite3_finalize(stmt);

        // close db
        int result = sqlite3_close(impl->conn);
        if (result != SQLITE_OK)
            LogWarning() << "Failed to close database";
        impl->conn = nullptr;
    }

    delete impl;
}

QueryImpl* SQLiteDb::newQuery()
{
    return new SQLiteQueryImpl(this);
}

std::string SQLiteDb::getCreateTableQuery(const Entity& entity) const
{
    std::string fieldsStr;

    for (const Field& field : entity.getFields()) {
        if (!fieldsStr.empty())
            fieldsStr += ", ";
        fieldsStr += field.name + " ";
        fieldsStr += getTypeName(field.type);
        fieldsStr += " ";
        if (field.isPK)
            fieldsStr += "PRIMARY KEY ";
        if (field.isAutoincrement)
            fieldsStr += "AUTOINCREMENT ";
        if (field.isUnique)
            fieldsStr += "UNIQUE ";
        if (field.notNull)
            fieldsStr += "NOT NULL ";
        if (!field.defaultValue.empty()) {
            if (field.type == Field::DataType::String) {
                fieldsStr += "DEFAULT \"" + field.defaultValue + "\"";
            } else {
                fieldsStr += "DEFAULT " + field.defaultValue;
            }
        }
    }

    return "CREATE TABLE IF NOT EXISTS " + entity.getTableName() + " (" + fieldsStr + ")";
}

const std::string& SQLiteDb::getTypeName(Field::DataType type) const
{
    static const int size = static_cast<int>(Field::DataType::Last);
    static const std::string types[size] = {
        "VOID",
        "BOOLEAN",
        "INTEGER",
        "BIGINT",
        "DOUBLE",
        "INTEGER",
        "TEXT"
    };

    const int pos = static_cast<int>(type);
    return (pos >= size || pos <= 0) ? types[0] : types[pos];
}

} // namespace ngrest
