#pragma once

#include "../sql.hpp"

namespace pg {
    template <>
    struct type<bool> {
        static constexpr std::int32_t oid = 16;
        static constexpr std::string_view name = "bool";

        static auto from_sql(
            std::int32_t size,
            netcore::buffered_socket& reader
        ) -> ext::task<bool>;

        static auto to_sql(bool b, netcore::buffered_socket& writer)
            -> ext::task<>;

        static constexpr auto size(bool b) -> std::int32_t {
            return sizeof(bool);
        }
    };

    static_assert(sql_type<bool>);
    static_assert(from_sql<bool>);
    static_assert(to_sql<bool>);
}
