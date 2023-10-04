#pragma once

#include "../reader.hpp"
#include "../writer.hpp"

#include <ext/bit>

namespace pg::detail {
    template <std::signed_integral T>
    struct decoder<T> {
        static auto decode(netcore::buffered_socket& reader) -> ext::task<T> {
            T result = 0;

            co_await reader.read(&result, sizeof(T));
            result = ext::from_be(result);

            TIMBER_TRACE("read Int{}({})", sizeof(T) * 8, result);

            co_return result;
        }
    };

    template <std::signed_integral T>
    struct encoder<T> {
        static auto encode(T t, netcore::buffered_socket& writer)
            -> ext::task<> {
            TIMBER_TRACE("write Int{}({})", sizeof(T) * 8, t);

            const auto be = ext::to_be(t);
            co_await writer.write(&be, sizeof(T));
        }

        static auto size(T t) -> std::int32_t { return sizeof(T); }
    };

    static_assert(decodable<std::int8_t>);
    static_assert(decodable<std::int16_t>);
    static_assert(decodable<std::int32_t>);
    static_assert(decodable<std::int64_t>);

    static_assert(encodable<std::int8_t>);
    static_assert(encodable<std::int16_t>);
    static_assert(encodable<std::int32_t>);
    static_assert(encodable<std::int64_t>);
}
