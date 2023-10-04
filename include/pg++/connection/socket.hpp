#pragma once

#include "reader.hpp"
#include "type/byte.hpp"
#include "type/header.hpp"
#include "type/int.hpp"
#include "type/sql.hpp"
#include "writer.hpp"

namespace pg::detail {
    class socket final {
        friend struct fmt::formatter<pg::detail::socket>;

        netcore::buffered_socket inner;
    public:
        template <typename... Args>
        socket(Args&&... args) : inner(std::forward<Args>(args)...) {}

        socket(socket&& other);

        auto operator=(socket&& other) -> socket&;

        auto cancel() noexcept -> void;

        auto flush() -> ext::task<>;

        template <composite_type T>
        auto from_row(std::int16_t fields) -> ext::task<T> {
            auto fields32 = static_cast<std::int32_t>(fields);
            co_return co_await type<T>::from_row(fields32, inner);
        }

        template <pg::from_sql T>
        auto from_sql() -> ext::task<T> {
            co_return co_await detail::from_sql<T>(inner);
        }

        template <encodable... Args>
        auto message(char identifier, Args&&... args) -> ext::task<> {
            co_await write(identifier);
            co_await send(std::forward<Args>(args)...);
        }

        template <decodable T>
        auto read() -> ext::task<T> {
            return decoder<T>::decode(inner);
        }

        auto read_header() -> ext::task<header>;

        template <encodable... Args>
        auto send(Args&&... args) -> ext::task<> {
            const auto size = detail::size(std::forward<Args>(args)...);
            co_await write(size);

            (co_await write(args), ...);
        }

        template <encodable T>
        auto write(const T& t) -> ext::task<> {
            return encoder<T>::encode(t, inner);
        }
    };
}

template <>
struct fmt::formatter<pg::detail::socket> :
    formatter<netcore::buffered_socket> {
    template <typename FormatContext>
    auto format(const pg::detail::socket& socket, FormatContext& ctx) {
        return formatter<netcore::buffered_socket>::format(socket.inner, ctx);
    }
};
