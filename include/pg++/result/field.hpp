#pragma once

#include "column.hpp"

#include <pg++/connection/reader.hpp>

#include <memory>
#include <optional>
#include <vector>
#include <span>

namespace pg::detail {
    class field_data {
        std::unique_ptr<std::byte[]> data;
        std::size_t _size = 0;
    public:
        explicit operator bool() const noexcept;

        field_data() = default;

        field_data(std::int32_t size);

        auto get() const noexcept -> const std::byte*;

        auto get() noexcept -> std::byte*;

        auto size() const noexcept -> std::size_t;
    };

    template <>
    struct decoder<field_data> {
        static auto decode(reader& reader) -> ext::task<field_data>;
    };

    static_assert(decodable<field_data>);
}

namespace pg {
    class field {
        std::reference_wrapper<const detail::column> column;
        detail::field_data data;
    public:
        field(const detail::column& column, detail::field_data&& data);

        auto bytes() const noexcept -> std::span<const std::byte>;

        auto is_null() const noexcept -> bool;

        auto name() const noexcept -> std::string_view;

        auto string() const noexcept -> std::optional<std::string_view>;
    };

    class row {
        std::span<const detail::column> columns;
        std::vector<field> fields;
    public:
        using iterator = std::vector<field>::const_iterator;

        row(
            std::span<const detail::column> columns,
            std::vector<field>&& fields
        );

        auto operator[](std::size_t index) const noexcept -> const field&;

        auto operator[](
            std::string_view name
        ) const -> const field&;

        auto begin() const noexcept -> iterator;

        auto end() const noexcept -> iterator;

        auto size() const noexcept -> std::size_t;
    };
}
