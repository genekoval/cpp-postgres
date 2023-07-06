#pragma once

#include "io.hpp"

#include <netcore/netcore>

namespace pg {
    template <typename T>
    concept to_sql = requires(const T& t, netcore::buffered_socket& w) {
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
    concept encodable = requires(const T& t, netcore::buffered_socket& w) {
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
