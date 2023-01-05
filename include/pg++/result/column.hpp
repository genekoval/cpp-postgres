#pragma once

#include <pg++/connection/reader.hpp>

#include <string>

namespace pg::detail {
    enum class format_code : std::int16_t {
        text,
        binary
    };

    struct column {
        std::string name;
        std::int32_t table;
        std::int16_t column;
        std::int32_t type;
        std::int16_t size;
        std::int32_t modifier;
        format_code format;
    };

    template <>
    struct decoder<column> {
        static auto decode(reader& reader) -> ext::task<column>;
    };

    static_assert(decodable<column>);
}
