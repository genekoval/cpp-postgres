#pragma once

#include "span.hpp"

namespace pg {
    template <typename T>
    class type<std::vector<T>> {
        using span = type<std::span<const T>>;
    public:
        static inline std::int32_t oid = -1;

        static auto oid_query(detail::connection& connection) -> ext::task<> {
            if (span::oid == -1) co_await span::oid_query(connection);
            oid = span::oid;
        }

        static auto from_sql(
            std::int32_t size,
            netcore::buffered_socket& reader
        ) -> ext::task<std::vector<T>> {
            using int32 = detail::decoder<std::int32_t>;

            static_assert(pg::from_sql<T>);

            const auto dimensions = co_await int32::decode(reader);

            if (dimensions > 1)
                throw bad_conversion(
                    fmt::format("invalid number of dimensions: {}", dimensions)
                );

            // read flags
            co_await int32::decode(reader);
            // read element type
            co_await int32::decode(reader);

            if (dimensions == 0) { co_return std::vector<T>(); }

            const auto len = co_await int32::decode(reader);

            // read lower bound
            co_await int32::decode(reader);

            auto result = std::vector<T>();
            result.reserve(len);

            for (auto i = 0; i < len; ++i) {
                result.push_back(co_await detail::from_sql<T>(reader));
            }

            co_return result;
        }

        static auto to_sql(
            std::span<const T> span,
            netcore::buffered_socket& writer
        ) -> ext::task<> {
            return span::to_sql(span, writer);
        }

        static auto size(std::span<const T> span) -> std::int32_t {
            return span::size(span);
        }

        static auto null() noexcept -> std::vector<T> {
            return std::vector<T>();
        }

        static constexpr auto is_null(const std::vector<T>&) noexcept -> bool {
            return false;
        }
    };

    static_assert(sql_type<std::vector<std::int32_t>>);
    static_assert(from_sql<std::vector<std::int32_t>>);
    static_assert(to_sql<std::vector<std::int32_t>>);
    static_assert(nullable<std::vector<std::int32_t>>);
}
