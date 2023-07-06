#pragma once

#include "io.hpp"

#include <netcore/netcore>

namespace pg {
    template <typename T>
    concept from_sql = requires(
        std::int32_t size,
        netcore::buffered_socket& r
    ) {
        { type<std::remove_cvref_t<T>>::from_sql(size, r) } ->
            std::same_as<ext::task<std::remove_cvref_t<T>>>;
    };

    template <typename T>
    concept composite_type = requires(
        std::int32_t& fields,
        netcore::buffered_socket& reader
    ) {
        { type<std::remove_cvref_t<T>>::from_row(fields, reader) } ->
            std::same_as<ext::task<std::remove_cvref_t<T>>>;
    };
}

namespace pg::detail {
    template <typename T>
    struct decoder {};

    template <typename T>
    concept decodable = requires(netcore::buffered_socket& r) {
        { decoder<std::remove_cvref_t<T>>::decode(r) } ->
            std::same_as<ext::task<std::remove_cvref_t<T>>>;
    };
}
