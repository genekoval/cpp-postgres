#pragma once

#include "../reader.hpp"
#include "../writer.hpp"

namespace pg::detail {
    struct notification {
        std::int32_t pid;
        std::string channel;
        std::string payload;
    };

    template <>
    struct decoder<notification> {
        static auto decode(
            netcore::buffered_socket& reader
        ) -> ext::task<notification>;
    };

    static_assert(decodable<notification>);
}
