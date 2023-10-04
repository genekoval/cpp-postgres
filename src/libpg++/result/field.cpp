#include <pg++/connection/type/int.hpp>
#include <pg++/except/except.hpp>
#include <pg++/result/field.hpp>

#include <cassert>
#include <fmt/format.h>

namespace pg::detail {
    field_data::field_data(std::int32_t size) :
        data(new std::byte[size]),
        _size(size) {}

    field_data::operator bool() const noexcept {
        return static_cast<bool>(data);
    }

    auto field_data::get() const noexcept -> const std::byte* {
        return data.get();
    }

    auto field_data::get() noexcept -> std::byte* { return data.get(); }

    auto field_data::size() const noexcept -> std::size_t { return _size; }

    auto decoder<field_data>::decode(netcore::buffered_socket& reader)
        -> ext::task<field_data> {
        const auto size = co_await decoder<std::int32_t>::decode(reader);

        if (size == -1) co_return field_data();

        auto result = field_data(size);
        co_await reader.read(result.get(), size);

        co_return result;
    }
}

namespace pg {
    field::field(const detail::column& column, detail::field_data&& data) :
        column(column),
        data(std::forward<detail::field_data>(data)) {}

    auto field::bytes() const noexcept -> std::span<const std::byte> {
        if (!data) return {};
        return {data.get(), data.size()};
    }

    auto field::is_null() const noexcept -> bool { return !data; }

    auto field::name() const noexcept -> std::string_view {
        return column.get().name;
    }

    auto field::string() const noexcept -> std::optional<std::string_view> {
        if (!data) return {};

        return std::string_view(
            reinterpret_cast<const char*>(data.get()),
            data.size()
        );
    }

    auto field::type() const noexcept -> std::int32_t {
        return column.get().type;
    }

    row::row(
        std::span<const detail::column> columns,
        std::vector<field>&& fields
    ) :
        columns(columns),
        fields(std::forward<std::vector<field>>(fields)) {}

    auto row::operator[](std::size_t index) const noexcept -> const field& {
        assert(index < fields.size());
        return fields[index];
    }

    auto row::operator[](std::string_view name) const -> const field& {
        for (std::size_t i = 0; i < columns.size(); ++i) {
            if (columns[i].name == name) return fields[i];
        }

        throw error(fmt::format(R"(column "{}" does not exist)", name));
    }

    auto row::begin() const noexcept -> iterator { return fields.begin(); }

    auto row::end() const noexcept -> iterator { return fields.end(); }

    auto row::size() const noexcept -> std::size_t { return fields.size(); }
}
