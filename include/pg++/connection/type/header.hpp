#pragma once

#include <cstdint>

#include "../reader.hpp"
#include "../writer.hpp"

namespace pg::detail {
    struct header {
        char code;
        std::int32_t len;
    };

    template <>
    struct decoder<header> {
        static auto decode(
            netcore::buffered_socket& reader
        ) -> ext::task<header>;
    };

    static_assert(decodable<header>);
}
