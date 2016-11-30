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

#ifndef NGREST_QUERYIMPL_H
#define NGREST_QUERYIMPL_H

#include <string>

namespace ngrest {

class QueryImpl
{
public:
    virtual ~QueryImpl();

    virtual void reset() = 0;

    inline bool query(const std::string& query)
    {
        prepare(query);
        return next();
    }

    virtual void prepare(const std::string& query) = 0;
    virtual void bindNull(int arg) = 0;
    virtual void bindBool(int arg, bool value) = 0;
    virtual void bindInt(int arg, int value) = 0;
    virtual void bindBigInt(int arg, int64_t value) = 0;
    virtual void bindFloat(int arg, double value) = 0;
    virtual void bindString(int arg, const std::string& value) = 0;

    virtual bool next() = 0;
    virtual bool resultIsNull(int column) = 0;
    virtual bool resultBool(int column) = 0;
    virtual int resultInt(int column) = 0;
    virtual int64_t resultBigInt(int column) = 0;
    virtual double resultFloat(int column) = 0;
    virtual void resultString(int column, std::string& value) = 0;

    virtual int64_t lastInsertId() = 0;
};

} // namespace ngrest

#endif // NGREST_QUERYIMPL_H
