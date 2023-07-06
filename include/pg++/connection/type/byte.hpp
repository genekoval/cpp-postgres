#pragma once

#include "../reader.hpp"
#include "../writer.hpp"

namespace pg::detail {
    template <>
    struct decoder<char> {
        static auto decode(netcore::buffered_socket& reader) -> ext::task<char>;
    };

    template <>
    struct encoder<char> {
        static auto encode(
            char c,
            netcore::buffered_socket& writer
        ) -> ext::task<>;

        static auto size(char c) -> std::int32_t;
    };

    static_assert(decodable<char>);
    static_assert(encodable<char>);
}
