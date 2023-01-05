#pragma once

#include <pg++/connection/connection.hpp>

namespace pg {
    class portal {
        std::reference_wrapper<detail::connection> connection;
        std::string name;
        std::vector<detail::column> columns;
        std::vector<row> rows;
        std::int32_t max_rows;
        std::optional<std::string> tag;
    public:
        portal(
            detail::connection& connection,
            std::string_view name,
            std::vector<detail::column>&& columns,
            std::int32_t max_rows
        );

        explicit operator bool() const noexcept;

        auto done() const noexcept -> bool;

        auto next() -> ext::task<std::span<const row>>;

        auto command_tag() const noexcept -> std::optional<std::string_view>;
    };
}
