#pragma once

#include "../sql.hpp"

#include <chrono>

namespace pg {
    using timestamp = std::chrono::system_clock::time_point;

    template <>
    struct type<timestamp> {
        static constexpr std::int32_t oid = 1184;
        static constexpr std::string_view name = "timestamptz";

        static auto from_sql(
            std::int32_t size,
            reader& reader
        ) -> ext::task<timestamp>;

        static auto to_sql(
            const timestamp time,
            writer& writer
        ) -> ext::task<>;

        static constexpr auto size(const timestamp time) -> std::int32_t {
            return sizeof(std::int64_t);
        }
    };
}
