#pragma once

#include "../sql.hpp"

namespace pg {
    template <>
    struct type<std::string> {
        static constexpr std::int32_t oid = 25;
        static constexpr std::string_view name = "text";

        static auto from_sql(
            std::int32_t size,
            netcore::buffered_socket& reader
        ) -> ext::task<std::string>;

        static auto to_sql(
            std::string_view string,
            netcore::buffered_socket& writer
        ) -> ext::task<>;

        static auto size(std::string_view string) -> std::int32_t;
    };

    static_assert(sql_type<std::string>);
    static_assert(from_sql<std::string>);
    static_assert(to_sql<std::string>);

    template <>
    struct type<std::string_view> {
        static constexpr std::int32_t oid = type<std::string>::oid;
        static constexpr std::string_view name = type<std::string>::name;

        static auto to_sql(
            std::string_view string,
            netcore::buffered_socket& writer
        ) -> ext::task<>;

        static auto size(std::string_view string) -> std::int32_t;
    };

    static_assert(sql_type<std::string_view>);
    static_assert(to_sql<std::string_view>);

    template <>
    struct type<const char*> {
        static constexpr std::int32_t oid = type<std::string>::oid;
        static constexpr std::string_view name = type<std::string>::name;

        static auto to_sql(const char* string, netcore::buffered_socket& writer)
            -> ext::task<>;

        static auto size(const char* string) -> std::int32_t;
    };

    static_assert(sql_type<const char*>);
    static_assert(to_sql<const char*>);

    template <std::size_t N>
    struct type<char[N]> {
        static constexpr std::int32_t oid = type<std::string>::oid;
        static constexpr std::string_view name = type<std::string>::name;

        static auto to_sql(const char* string, netcore::buffered_socket& writer)
            -> ext::task<> {
            co_await type<std::string_view>::to_sql(string, writer);
        }

        static auto size(const char* string) -> std::int32_t {
            return type<std::string_view>::size(string);
        }
    };

    static_assert(sql_type<char[10]>);
    static_assert(to_sql<char[10]>);
}
