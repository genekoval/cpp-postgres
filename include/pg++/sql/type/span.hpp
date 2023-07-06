#pragma once

#include "../sql.hpp"

#include <pg++/connection/connection.hpp>
#include <pg++/connection/type/int.hpp>
#include <pg++/connection/type/sql.hpp>

#include <numeric>

namespace pg {
    template <typename T>
    struct type<std::span<const T>> {
        static inline std::int32_t oid = -1;

        static auto oid_query(detail::connection& connection) -> ext::task<> {
            const auto element_type = co_await connection.get_oid<T>();

            oid = co_await connection.fetch<std::int32_t>(
                "SELECT typarray FROM pg_type WHERE oid = $1",
                element_type
            );
        }

        static auto to_sql(
            std::span<const T> span,
            netcore::buffered_socket& writer
        ) -> ext::task<> {
            using int32 = detail::encoder<std::int32_t>;

            // write dimensions
            co_await int32::encode(span.empty() ? 0 : 1, writer);

            auto has_nulls = false;

            if constexpr (nullable<T>) {
                auto i = 0;

                while (!has_nulls && i < span.size()) {
                    has_nulls = type<T>::is_null(span[i++]);
                }
            }

            co_await int32::encode(has_nulls ? 1 : 0, writer);
            co_await int32::encode(type<T>::oid, writer);

            if (span.empty()) co_return;

            co_await int32::encode(span.size(), writer);
            co_await int32::encode(1, writer); // lower bound

            for (const auto& element : span) {
                co_await detail::encoder<detail::sql_type<T>>::encode(
                    element,
                    writer
                );
            }
        }

        static auto size(std::span<const T> span) -> std::int32_t {
            if (span.empty()) {
                // 1. dimensions
                // 2. flags
                // 3. type
                return sizeof(std::int32_t) * 3;
            }

            // 1. dimensions
            // 2. flags
            // 3. type
            // 4. length
            // 5. lower bound
            constexpr std::int32_t header = sizeof(std::int32_t) * 5;

            return std::transform_reduce(
                span.begin(),
                span.end(),
                header,
                std::plus<>(),
                [](const T& t) {
                    return detail::encoder<detail::sql_type<T>>::size(t);
                }
            );
        }
    };

    static_assert(sql_type<std::span<const std::int32_t>>);
    static_assert(to_sql<std::span<const std::int32_t>>);
}
