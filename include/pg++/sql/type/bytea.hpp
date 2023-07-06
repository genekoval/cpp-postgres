#pragma once

#include "../sql.hpp"

namespace pg {
    class bytea {
        std::unique_ptr<std::byte[]> bytes;
        std::size_t count = 0;
    public:
        bytea() = default;

        bytea(std::size_t size);

        bytea(std::span<const std::byte> bytes);

        auto data() noexcept -> std::byte*;

        auto data() const noexcept -> const std::byte*;

        auto size() const noexcept -> std::size_t;

        auto span() noexcept -> std::span<std::byte>;

        auto span() const noexcept -> std::span<const std::byte>;
    };

    template <>
    struct type<bytea> {
        static constexpr std::int32_t oid = 17;
        static constexpr std::string_view name = "bytea";

        static auto from_sql(
            std::int32_t size,
            netcore::buffered_socket& reader
        ) -> ext::task<bytea>;

        static auto to_sql(
            const bytea& bytea,
            netcore::buffered_socket& writer
        ) -> ext::task<>;

        static auto size(const bytea& bytea) -> std::int32_t;
    };

    static_assert(sql_type<bytea>);
    static_assert(from_sql<bytea>);
    static_assert(to_sql<bytea>);

    template <>
    struct type<std::span<const std::byte>> {
        static constexpr std::int32_t oid = type<bytea>::oid;
        static constexpr std::string_view name = type<bytea>::name;

        static auto to_sql(
            std::span<const std::byte> bytes,
            netcore::buffered_socket& writer
        ) -> ext::task<>;

        static auto size(std::span<const std::byte> bytes) -> std::int32_t;
    };

    static_assert(sql_type<std::span<const std::byte>>);
    static_assert(to_sql<std::span<const std::byte>>);
}
