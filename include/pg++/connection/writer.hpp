#pragma once

#include "io.hpp"

namespace pg {
    class writer final : public detail::io {
        auto write_bytes(const std::byte* src, std::size_t len) -> ext::task<>;
    public:
        explicit writer(netcore::socket& socket);

        auto flush() -> ext::task<>;

        auto write(const void* src, std::size_t len) -> ext::task<>;
    };

    template <typename T>
    concept to_sql = requires(const T& t, writer& w) {
        { type<std::remove_cvref_t<T>>::to_sql(t, w) } ->
            std::same_as<ext::task<>>;

        { type<std::remove_cvref_t<T>>::size(t) } ->
            std::convertible_to<std::int32_t>;
    };
}

namespace pg::detail {
    template <typename T>
    struct encoder {};

    template <typename T>
    concept encodable = requires(const T& t, writer& w) {
        { encoder<std::remove_cvref_t<T>>::encode(t, w) } ->
            std::same_as<ext::task<>>;

        { encoder<std::remove_cvref_t<T>>::size(t) } ->
            std::convertible_to<std::int32_t>;
    };

    template <encodable... Args>
    auto size(Args&&... args) -> std::int32_t {
        return (
            sizeof(std::int32_t) + // size of length value itself
            ... + encoder<std::remove_cvref_t<Args>>::size(args)
        );
    }
}
