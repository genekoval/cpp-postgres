#pragma once

#include "../sql.hpp"

#include <pg++/connection/type/sql.hpp>

#define PGCPP_COMPOSITE_DECL(Type, Name)                                       \
    template <>                                                                \
    struct pg::type<Type> {                                                    \
        static inline std::int32_t oid = -1;                                   \
        static constexpr std::string_view name = Name;                         \
                                                                               \
        static auto from_row(                                                  \
            std::int32_t& fields,                                              \
            netcore::buffered_socket& reader                                   \
        ) -> ext::task<Type>;                                                  \
                                                                               \
        static auto from_sql(                                                  \
            std::int32_t size,                                                 \
            netcore::buffered_socket& reader                                   \
        ) -> ext::task<Type>;                                                  \
                                                                               \
        static auto to_sql(const Type& t, netcore::buffered_socket& writer)    \
            -> ext::task<>;                                                    \
                                                                               \
        static auto size(const Type& t) -> ::std::int32_t;                     \
    };

#define PGCPP_COMPOSITE_DEFINE(Type, ...)                                      \
    auto pg::type<Type>::from_row(                                             \
        std::int32_t& fields,                                                  \
        netcore::buffered_socket& reader                                       \
    )                                                                          \
        ->ext::task<Type> {                                                    \
        co_return co_await pg::composite<Type>::from_row(                      \
            fields,                                                            \
            reader,                                                            \
            __VA_ARGS__                                                        \
        );                                                                     \
    }                                                                          \
                                                                               \
    auto pg::type<Type>::from_sql(                                             \
        std::int32_t size,                                                     \
        netcore::buffered_socket& reader                                       \
    )                                                                          \
        ->ext::task<Type> {                                                    \
        co_return co_await pg::composite<Type>::from_sql(reader, __VA_ARGS__); \
    }                                                                          \
                                                                               \
    auto pg::type<Type>::to_sql(                                               \
        const Type& t,                                                         \
        netcore::buffered_socket& writer                                       \
    )                                                                          \
        ->ext::task<> {                                                        \
        co_await pg::composite<Type>::to_sql(t, writer, __VA_ARGS__);          \
    }                                                                          \
                                                                               \
    auto pg::type<Type>::size(const Type& t)->::std::int32_t {                 \
        return pg::composite<Type>::size(t, __VA_ARGS__);                      \
    }

namespace pg {
    template <std::default_initializable T>
    class composite {
        template <typename Type, typename Base>
        static auto read_member(
            T& t,
            std::int32_t oid,
            std::int32_t& fields,
            netcore::buffered_socket& reader,
            Type Base::*member
        ) -> ext::task<> {
            t.*member = co_await detail::from_sql<Type>(reader);
        }

        template <typename Type, typename Base>
        static auto read_member(
            T& t,
            std::int32_t& fields,
            netcore::buffered_socket& reader,
            Type Base::*member
        ) -> ext::task<> {
            const auto oid =
                co_await detail::decoder<std::int32_t>::decode(reader);

            co_await read_member(t, oid, fields, reader, member);
        }

        template <typename Type, typename Base>
        static auto write_member(
            const T& t,
            netcore::buffered_socket& writer,
            Type Base::*member
        ) -> ext::task<> {
            co_await detail::encoder<std::int32_t>::encode(
                type<T>::oid,
                writer
            );

            co_await detail::encoder<detail::sql_type<Type>>::encode(
                t.*member,
                writer
            );
        }

        template <typename Type, typename Base>
        static auto size(const T& t, Type Base::*member) -> std::int32_t {
            return detail::encoder<detail::sql_type<Type>>::size(t.*member);
        }
    public:
        template <typename... Members>
        static auto from_row(
            std::int32_t& fields,
            netcore::buffered_socket& reader,
            Members... members
        ) -> ext::task<T> {
            auto result = T();

            (co_await read_member(result, 0, fields, reader, members), ...);

            co_return result;
        }

        template <typename... Members>
        static auto from_sql(
            netcore::buffered_socket& reader,
            Members... members
        ) -> ext::task<T> {
            auto result = T();
            auto fields =
                co_await detail::decoder<std::int32_t>::decode(reader);

            (co_await read_member(result, fields, reader, members), ...);

            co_return result;
        }

        template <typename... Members>
        static auto to_sql(
            const T& t,
            netcore::buffered_socket& writer,
            Members... members
        ) -> ext::task<> {
            co_await detail::encoder<std::int32_t>::encode(
                sizeof...(Members),
                writer
            );

            (co_await write_member(t, writer, members), ...);
        }

        template <typename... Members>
        static auto size(const T& t, Members... members) -> std::int32_t {
            return sizeof(std::int32_t) + // size of field count
                   sizeof(std::int32_t) * sizeof...(Members) + // size of OIDs
                   (size(t, members) + ...);
        }
    };
}
