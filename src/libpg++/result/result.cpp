#include <pg++/result/result.hpp>

#include <cassert>

namespace pg {
    result::result(
        std::string_view tag,
        std::vector<detail::column>&& columns,
        std::vector<row>&& rows
    ) :
        tag(tag),
        columns(std::forward<std::vector<detail::column>>(columns)),
        rows(std::forward<std::vector<row>>(rows))
    {}

    auto result::operator[](
        std::size_t index
    ) const noexcept -> const row& {
        assert(index < rows.size());
        return rows[index];
    }

    auto result::begin() const noexcept -> iterator {
        return rows.begin();
    }

    auto result::command_tag() const noexcept -> std::string_view {
        return tag;
    }

    auto result::empty() const noexcept -> bool {
        return rows.empty();
    }

    auto result::end() const noexcept -> iterator {
        return rows.end();
    }

    auto result::size() const noexcept -> std::size_t {
        return rows.size();
    }
}
