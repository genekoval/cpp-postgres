#pragma once

#include "../sql.hpp"

#include <pg++/connection/type/sql.hpp>

#define PG_COMPOSITE(Type, ...) \
    template <> \
    struct ::pg::type<Type> { \
        static auto from_row( \
            std::int32_t& fields, \
            ::pg::reader& reader \
        ) -> ext::task<Type> { \
            co_return co_await ::pg::composite<Type>::from_row( \
                fields, \
                reader, \
                __VA_ARGS__ \
            ); \
        } \
\
        static auto from_sql( \
            std::int32_t size, \
            ::pg::reader& reader \
        ) -> ext::task<Type> { \
            co_return co_await ::pg::composite<Type>::from_sql( \
                reader, \
                __VA_ARGS__ \
            ); \
        } \
    };

namespace pg {
    template <std::default_initializable T>
    class composite {
        template <typename Type, typename Base>
        static auto read_member(
            T& t,
            std::int32_t oid,
            std::int32_t& fields,
            reader& reader,
            Type Base::* member
        ) -> ext::task<> {
            t.*member = co_await detail::from_sql<Type>(reader);
        }

        template <typename Type, typename Base>
        static auto read_member(
            T& t,
            std::int32_t& fields,
            reader& reader,
            Type Base::* member
        ) -> ext::task<> {
            const auto oid =
                co_await detail::decoder<std::int32_t>::decode(reader);

            co_await read_member(t, oid, fields, reader, member);
        }
    public:
        template <typename... Members>
        static auto from_row(
            std::int32_t& fields,
            reader& reader,
            Members... members
        ) -> ext::task<T> {
            auto result = T();

            (co_await read_member(result, 0, fields, reader, members), ...);

            co_return result;
        }

        template <typename... Members>
        static auto from_sql(
            reader& reader,
            Members... members
        ) -> ext::task<T> {
            auto result = T();
            auto fields =
                co_await detail::decoder<std::int32_t>::decode(reader);

            (co_await read_member(result, fields, reader, members), ...);

            co_return result;
        }
    };
}
