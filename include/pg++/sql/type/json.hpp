#pragma once

#include "../sql.hpp"

#include <nlohmann/json.hpp>

namespace pg {
    using json = nlohmann::json;

    template <>
    struct type<json> {
        static constexpr std::int32_t oid = 3802;
        static constexpr std::string_view name = "jsonb";
        static constexpr std::int8_t version = 1;

        static auto from_sql(
            std::int32_t size,
            netcore::buffered_socket& reader
        ) -> ext::task<json>;

        static auto to_sql(
            const json& j,
            netcore::buffered_socket& writer
        ) -> ext::task<>;

        static auto size(const json& j) -> std::int32_t;
    };

    static_assert(sql_type<json>);
    static_assert(from_sql<json>);
    static_assert(to_sql<json>);
}
