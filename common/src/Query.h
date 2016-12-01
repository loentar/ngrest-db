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

#ifndef NGREST_QUERY_H
#define NGREST_QUERY_H

#include <string>
#include <tuple>

#include <ngrest/common/Nullable.h>

#include "QueryImpl.h"

namespace ngrest {

class Db;

class Query
{
public:
    Query(Db& db);
    Query(QueryImpl* impl);

    ~Query();


    inline void reset()
    {
        impl->reset();
    }

    inline bool query(const std::string& query)
    {
        return impl->query(query);
    }

    inline void prepare(const std::string& query)
    {
        impl->prepare(query);
    }


    inline void bindNull(int arg)
    {
        impl->bindNull(arg);
    }

    inline void bind(int arg, std::nullptr_t)
    {
        impl->bindNull(arg);
    }

    inline void bind(int arg, bool value)
    {
        impl->bindBool(arg, value);
    }

    inline void bind(int arg, char value)
    {
        impl->bindInt(arg, value);
    }

    inline void bind(int arg, int value)
    {
        impl->bindInt(arg, value);
    }

    inline void bind(int arg, short value)
    {
        impl->bindInt(arg, value);
    }

    inline void bind(int arg, long value)
    {
        impl->bindBigInt(arg, value);
    }

    inline void bind(int arg, long long value)
    {
        impl->bindBigInt(arg, value);
    }


    inline void bind(int arg, unsigned char value)
    {
        impl->bindInt(arg, static_cast<int>(value));
    }

    inline void bind(int arg, unsigned int value)
    {
        impl->bindBigInt(arg, static_cast<int64_t>(value));
    }

    inline void bind(int arg, unsigned short value)
    {
        impl->bindInt(arg, static_cast<int>(value));
    }

    inline void bind(int arg, unsigned long value)
    {
        impl->bindBigInt(arg, static_cast<int64_t>(value));
    }

    inline void bind(int arg, unsigned long long value)
    {
        impl->bindBigInt(arg, static_cast<int64_t>(value));
    }


    inline void bind(int arg, float value)
    {
        impl->bindFloat(arg, value);
    }

    inline void bind(int arg, double value)
    {
        impl->bindFloat(arg, value);
    }

    inline void bind(int arg, const char* value)
    {
        impl->bindString(arg, value);
    }

    inline void bind(int arg, const std::string& value)
    {
        impl->bindString(arg, value);
    }

    template <typename T>
    inline void bind(int arg, const Nullable<T>& value)
    {
        if (value.isNull()) {
            impl->bindNull(arg);
        } else {
            bind(arg, *value);
        }
    }


    template <typename... Params>
    inline void bindAll(const Params&... params)
    {
        bindNext(0, params...);
    }


    inline bool next()
    {
        return impl->next();
    }

    bool resultIsNull(int column)
    {
        return impl->resultIsNull(column);
    }


    inline void result(int column, bool& value)
    {
        value = impl->resultBool(column);
    }

    inline void result(int column, char& value)
    {
        value = static_cast<char>(impl->resultInt(column));
    }

    inline void result(int column, int& value)
    {
        value = impl->resultInt(column);
    }

    inline void result(int column, short& value)
    {
        value = static_cast<short>(impl->resultInt(column));
    }

    inline void result(int column, long& value)
    {
        value = static_cast<long>(impl->resultBigInt(column));
    }

    inline void result(int column, long long& value)
    {
        value = static_cast<long long>(impl->resultBigInt(column));
    }


    inline void result(int column, unsigned char& value)
    {
        value = static_cast<unsigned char>(impl->resultInt(column));
    }

    inline void result(int column, unsigned int& value)
    {
        value = impl->resultBigInt(column);
    }

    inline void result(int column, unsigned short& value)
    {
        value = static_cast<unsigned short>(impl->resultInt(column));
    }

    inline void result(int column, unsigned long& value)
    {
        value = static_cast<unsigned long>(impl->resultBigInt(column));
    }

    inline void result(int column, unsigned long long& value)
    {
        value = static_cast<unsigned long long>(impl->resultBigInt(column));
    }


    inline void result(int column, float& value)
    {
        value = static_cast<float>(impl->resultFloat(column));
    }

    inline void result(int column, double& value)
    {
        value = impl->resultFloat(column);
    }

    inline void result(int column, std::string& value)
    {
        impl->resultString(column, value);
    }

    template <typename T>
    inline void result(int column, Nullable<T>& value)
    {
        if (impl->resultIsNull(column)) {
            value.setNull();
        } else {
            result(column, value.get());
        }
    }


    template <typename... Types>
    inline void resultAll(std::tuple<Types...>& tuple)
    {
        resultNext<std::tuple<Types...>, Types...>(tuple);
    }

    template <typename Type>
    inline void resultAll(Type& value)
    {
        result(0, value);
    }

    inline bool resultBool(int column)
    {
        return impl->resultBool(column);
    }

    inline int resultInt(int column)
    {
        return impl->resultInt(column);
    }

    inline int64_t resultBigInt(int column)
    {
        return impl->resultBigInt(column);
    }

    inline double resultFloat(int column)
    {
        return impl->resultFloat(column);
    }

    inline void resultString(int column, std::string& value)
    {
        impl->resultString(column, value);
    }

    inline QueryImpl* take()
    {
        QueryImpl* res = impl;
        impl = nullptr;
        return res;
    }

    inline int64_t lastInsertId()
    {
        return impl->lastInsertId();
    }

private:
    inline void bindNext(int)
    {
    }

    template <typename Param1, typename... Params>
    inline void bindNext(int index, const Param1& param1, const Params&... params)
    {
        bind(index, param1);
        bindNext(index + 1, params...);
    }


    template<typename Tuple, size_t pos>
    inline void resultItem(Tuple& tuple)
    {
        result(pos, std::get<pos>(tuple));
    }

    template <typename Tuple>
    inline void resultNext(Tuple&)
    {
    }

    template <typename Tuple, typename Arg, typename...Args>
    inline void resultNext(Tuple& tuple)
    {
        resultNext<Tuple, Args...>(tuple);
        resultItem<Tuple, sizeof...(Args)>(tuple);
    }

private:
    QueryImpl* impl;
};

} // namespace ngrest

#endif // NGREST_QUERY_H
