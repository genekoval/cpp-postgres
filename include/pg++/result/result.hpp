#pragma once

#include "column.hpp"
#include "field.hpp"

#include <vector>

namespace pg {
    class result {
        std::string tag;
        std::vector<detail::column> columns;
        std::vector<row> rows;
    public:
        using iterator = std::vector<row>::const_iterator;

        result() = default;

        result(
            std::string_view tag,
            std::vector<detail::column>&& columns,
            std::vector<row>&& rows
        );

        auto operator[](std::size_t index) const noexcept -> const row&;

        auto begin() const noexcept -> iterator;

        auto command_tag() const noexcept -> std::string_view;

        auto empty() const noexcept -> bool;

        auto end() const noexcept -> iterator;

        auto size() const noexcept -> std::size_t;
    };
}
