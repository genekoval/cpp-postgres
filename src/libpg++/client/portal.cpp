#include <pg++/client/portal.hpp>

namespace pg {
    portal::portal(
        detail::connection& connection,
        std::string_view name,
        std::vector<detail::column>&& columns,
        std::int32_t max_rows
    ) :
        connection(connection),
        name(name),
        columns(std::forward<std::vector<detail::column>>(columns)),
        max_rows(max_rows)
    {}

    portal::operator bool() const noexcept { return tag.has_value(); }

    auto portal::command_tag() const noexcept ->
        std::optional<std::string_view> { return tag; }

    auto portal::done() const noexcept -> bool { return tag.has_value(); }

    auto portal::next() -> ext::task<std::span<const row>> {
        auto& conn = connection.get();

        tag = co_await conn.execute(
            name,
            columns,
            rows,
            max_rows
        );

        if (done()) {
            co_await conn.close_portal(name);
            co_await conn.sync();
        }

        co_return rows;
    }
}
