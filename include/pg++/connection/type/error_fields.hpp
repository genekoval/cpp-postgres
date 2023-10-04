#pragma once

#include "../reader.hpp"

#include <pg++/except/except.hpp>

namespace pg::detail {
    template <>
    struct decoder<error_fields> {
        static auto decode(netcore::buffered_socket& reader)
            -> ext::task<error_fields>;
    };

    static_assert(decodable<error_fields>);
}
