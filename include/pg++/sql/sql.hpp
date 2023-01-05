#pragma once

#include <pg++/connection/reader.hpp>
#include <pg++/connection/writer.hpp>
#include <pg++/except/except.hpp>

namespace pg {
    template <typename T>
    concept sql_type = requires {
        { type<std::remove_cvref_t<T>>::oid } ->
            std::convertible_to<std::int32_t>;
        { type<std::remove_cvref_t<T>>::name } ->
            std::convertible_to<std::string_view>;
    };

    template <typename T>
    concept nullable = requires(const T& t) {
        { type<std::remove_cvref_t<T>>::null() } ->
            std::same_as<std::remove_cvref_t<T>>;
        { type<std::remove_cvref_t<T>>::is_null(t) } -> std::same_as<bool>;
    };
}
