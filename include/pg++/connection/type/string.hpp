#pragma once

#include "../reader.hpp"
#include "../writer.hpp"

namespace pg::detail {
    template <>
    struct decoder<std::string> {
        static auto decode(
            netcore::buffered_socket& reader
        ) -> ext::task<std::string>;
    };

    template <>
    struct encoder<std::string> {
        static auto encode(
            std::string_view string,
            netcore::buffered_socket& writer
        ) -> ext::task<>;

        static auto size(std::string_view string) -> std::int32_t;
    };

    template <>
    struct encoder<std::string_view> {
        static auto encode(
            std::string_view string,
            netcore::buffered_socket& writer
        ) -> ext::task<>;

        static auto size(std::string_view string) -> std::int32_t;
    };

    static_assert(decodable<std::string>);
    static_assert(encodable<std::string>);

    static_assert(encodable<std::string_view>);
}
