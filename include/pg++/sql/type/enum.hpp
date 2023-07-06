#pragma once

#include "../sql.hpp"

#define PG_ENUM(Type, Name) \
    template <> \
    struct pg::type<Type> { \
        static_assert(pg::sql_enum<Type>); \
\
        static inline std::int32_t oid = -1; \
        static constexpr std::string_view name = Name; \
\
        static auto from_sql( \
            std::int32_t size, \
            netcore::buffered_socket& reader \
        ) -> ext::task<Type>; \
\
        static auto to_sql( \
            Type t, \
            netcore::buffered_socket& writer \
        ) -> ext::task<>; \
\
        static auto size(Type t) -> std::int32_t; \
    };

#define PG_ENUM_DEFINE(Type) \
    auto pg::type<Type>::from_sql( \
        std::int32_t size, \
        netcore::buffered_socket& reader \
    ) -> ext::task<Type> { \
        const auto string = co_await pg::type<std::string>::from_sql( \
            size, \
            reader \
        ); \
        co_return pg::enum_type<Type>::from_string(string); \
    } \
\
    auto pg::type<Type>::to_sql( \
        Type t, \
        netcore::buffered_socket& writer \
    ) -> ext::task<> { \
        const auto string = pg::enum_type<Type>::to_string(t); \
        co_await pg::type<std::string_view>::to_sql(string, writer); \
    } \
\
    auto pg::type<Type>::size(Type t) -> std::int32_t { \
        const auto string = pg::enum_type<Type>::to_string(t); \
        return string.size(); \
    }

namespace pg {
    template <typename T>
    struct enum_type {};

    template <typename T>
    concept sql_enum =
        std::is_enum_v<T> &&
        requires(
            T t,
            std::string_view string
        ) {
            { enum_type<std::remove_cvref_t<T>>::from_string(string) } ->
                std::convertible_to<T>;

            { enum_type<std::remove_cvref_t<T>>::to_string(t) } ->
                std::convertible_to<std::string_view>;
        };
}
