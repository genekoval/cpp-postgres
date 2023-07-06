#pragma once

#include "../sql.hpp"

namespace pg {
    template <sql_type T>
    struct type<std::optional<T>> {
        static constexpr std::int32_t oid = type<T>::oid;
        static constexpr std::string_view name = type<T>::name;

        static auto from_sql(
            std::int32_t size,
            netcore::buffered_socket& reader
        ) -> ext::task<std::optional<T>> {
            static_assert(pg::from_sql<T>);
            co_return co_await type<T>::from_sql(size, reader);
        }

        static auto to_sql(
            const std::optional<T>& opt,
            netcore::buffered_socket& writer
        ) -> ext::task<> {
            static_assert(pg::to_sql<T>);
            return type<T>::to_sql(*opt, writer);
        }

        static auto size(const std::optional<T>& opt) -> std::int32_t {
            static_assert(pg::to_sql<T>);
            return type<T>::size(*opt);
        }

        static constexpr auto null() noexcept -> std::optional<T> {
            return std::nullopt;
        }

        static constexpr auto is_null(
            const std::optional<T>& opt
        ) noexcept -> bool {
            return !opt;
        }
    };
}
