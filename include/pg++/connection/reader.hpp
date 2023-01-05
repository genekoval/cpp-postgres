#pragma once

#include "io.hpp"

namespace pg {
    class reader final : public detail::io {
        auto fill() -> ext::task<>;

        auto read_bytes(std::byte* dest, std::size_t len) -> ext::task<>;
    public:
        explicit reader(netcore::socket& socket);

        auto advance(std::size_t n) noexcept -> void;

        auto data() -> ext::task<std::span<const std::byte>>;

        auto read(void* dest, std::size_t len) -> ext::task<>;
    };

    template <typename T>
    concept from_sql = requires(std::int32_t size, reader& r) {
        { type<std::remove_cvref_t<T>>::from_sql(size, r) } ->
            std::same_as<ext::task<std::remove_cvref_t<T>>>;
    };

    template <typename T>
    concept composite_type = requires(std::int32_t& fields, reader& reader) {
        { type<std::remove_cvref_t<T>>::from_row(fields, reader) } ->
            std::same_as<ext::task<std::remove_cvref_t<T>>>;
    };
}

namespace pg::detail {
    template <typename T>
    struct decoder {};

    template <typename T>
    concept decodable = requires(reader& r) {
        { decoder<std::remove_cvref_t<T>>::decode(r) } ->
            std::same_as<ext::task<std::remove_cvref_t<T>>>;
    };
}
