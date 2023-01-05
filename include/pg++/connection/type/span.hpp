#pragma once

#include "int.hpp"

#include <numeric>

namespace pg::detail {
    template <encodable T>
    struct encoder<std::span<T>> {
        using size_type = std::int16_t;

        static auto encode(
            std::span<const T> span,
            writer& writer
        ) -> ext::task<> {
            co_await encoder<size_type>::encode(span.size(), writer);

            for (const auto& t : span) {
                co_await encoder<T>::encode(t, writer);
            }
        }

        static auto size(std::span<const T> span) -> std::int32_t {
            return std::transform_reduce(
                span.begin(),
                span.end(),
                static_cast<std::int32_t>(sizeof(size_type)),
                std::plus<>(),
                [](const T& t) { return encoder<T>::size(t); }
            );
        }
    };

    template <>
    struct encoder<std::span<const std::byte>> {
        using size_type = std::int32_t;

        static auto encode(
            std::span<const std::byte> bytes,
            writer& writer
        ) -> ext::task<> {
            co_await encoder<size_type>::encode(bytes.size(), writer);
            co_await writer.write(bytes.data(), bytes.size());
        }

        static auto size(std::span<const std::byte> bytes) -> std::int32_t {
            return sizeof(size_type) + bytes.size();
        }
    };

    static_assert(encodable<std::span<std::int32_t>>);
    static_assert(encodable<std::span<const std::byte>>);
}
