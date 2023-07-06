#pragma once

#include "../writer.hpp"

namespace pg::detail {
    template <typename T>
    requires
        std::is_enum_v<T> &&
        encodable<std::underlying_type_t<T>>
    struct encoder<T> {
        static auto encode(
            T t,
            netcore::buffered_socket& writer
        ) -> ext::task<> {
            using underlying = std::underlying_type_t<T>;

            co_await encoder<underlying>::encode(
                static_cast<underlying>(t),
                writer
            );
        }

        static auto size(T t) -> std::int32_t {
            return sizeof(T);
        }
    };
}
