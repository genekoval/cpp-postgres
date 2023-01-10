#pragma once

#include "../sql.hpp"

#include <uuid++/uuid++>

namespace pg {
    using uuid = UUID::uuid;

    template <>
    struct type<uuid> {
        static constexpr std::int32_t oid = 2950;
        static constexpr std::string_view name = "uuid";

        static auto from_sql(
            std::int32_t size,
            reader& reader
        ) -> ext::task<uuid>;

        static auto to_sql(const uuid& id, writer& writer) -> ext::task<>;

        static constexpr auto size(const uuid& id) -> std::int32_t {
            return UUID::size;
        }
    };

    static_assert(sql_type<uuid>);
    static_assert(from_sql<uuid>);
    static_assert(to_sql<uuid>);
}
