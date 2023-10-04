#pragma once

#include "../sql.hpp"

namespace pg {
    template <>
    struct type<std::int16_t> {
        static constexpr std::int32_t oid = 21;
        static constexpr std::string_view name = "int2";

        static auto from_sql(
            std::int32_t size,
            netcore::buffered_socket& reader
        ) -> ext::task<std::int16_t>;

        static auto to_sql(std::int16_t i, netcore::buffered_socket& writer)
            -> ext::task<>;

        static auto size(std::int16_t i) -> std::int32_t;
    };

    template <>
    struct type<std::int32_t> {
        static constexpr std::int32_t oid = 23;
        static constexpr std::string_view name = "int4";

        static auto from_sql(
            std::int32_t size,
            netcore::buffered_socket& reader
        ) -> ext::task<std::int32_t>;

        static auto to_sql(std::int32_t i, netcore::buffered_socket& writer)
            -> ext::task<>;

        static auto size(std::int32_t i) -> std::int32_t;
    };

    template <>
    struct type<std::int64_t> {
        static constexpr std::int32_t oid = 20;
        static constexpr std::string_view name = "int8";

        static auto from_sql(
            std::int32_t size,
            netcore::buffered_socket& reader
        ) -> ext::task<std::int64_t>;

        static auto to_sql(std::int64_t i, netcore::buffered_socket& writer)
            -> ext::task<>;

        static auto size(std::int64_t i) -> std::int32_t;
    };

    static_assert(sql_type<std::int16_t>);
    static_assert(sql_type<std::int32_t>);
    static_assert(sql_type<std::int64_t>);

    static_assert(from_sql<std::int16_t>);
    static_assert(from_sql<std::int32_t>);
    static_assert(from_sql<std::int64_t>);

    static_assert(to_sql<std::int16_t>);
    static_assert(to_sql<std::int32_t>);
    static_assert(to_sql<std::int64_t>);
}
