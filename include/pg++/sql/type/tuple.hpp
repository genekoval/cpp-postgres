#pragma once

#include <pg++/connection/type/int.hpp>
#include <pg++/connection/type/sql.hpp>
#include <pg++/except/except.hpp>

namespace pg {
    template <typename... Types>
    requires ((from_sql<Types> || composite_type<Types>), ...)
    class type<std::tuple<Types...>> {
        template <typename T>
        static auto read_field(
            T& t,
            std::int32_t oid,
            std::int32_t& fields,
            netcore::buffered_socket& reader
        ) -> ext::task<> {
            if (fields == 0) throw unexpected_data("missing field");
            --fields;

            t = co_await detail::from_sql<T>(reader);
        }

        template <typename T>
        static auto read_field(
            T& t,
            std::int32_t& fields,
            netcore::buffered_socket& reader
        ) -> ext::task<> {
            const auto oid =
                co_await detail::decoder<std::int32_t>::decode(reader);

            co_await read_field(t, oid, fields, reader);
        }

        template <std::size_t... I>
        static auto read_composite(
            std::index_sequence<I...>,
            std::tuple<Types...>& tuple,
            std::int32_t& fields,
            netcore::buffered_socket& reader
        ) -> ext::task<> {
            (co_await read_field(std::get<I>(tuple), fields, reader), ...);
        }

        template <std::size_t... I>
        static auto read_row(
            std::index_sequence<I...>,
            std::tuple<Types...>& tuple,
            std::int32_t& fields,
            netcore::buffered_socket& reader
        ) -> ext::task<> {
            (co_await read_field(std::get<I>(tuple), 0, fields, reader), ...);
        }
    public:
        static auto from_row(
            std::int32_t& fields,
            netcore::buffered_socket& reader
        ) -> ext::task<std::tuple<Types...>> {
            auto result = std::tuple<Types...>();

            co_await read_row(
                std::index_sequence_for<Types...>(),
                result,
                fields,
                reader
            );

            co_return result;
        }

        static auto from_sql(
            std::int32_t size,
            netcore::buffered_socket& reader
        ) -> ext::task<std::tuple<Types...>> {
            auto result = std::tuple<Types...>();
            auto fields =
                co_await detail::decoder<std::int32_t>::decode(reader);

            co_await read_composite(
                std::index_sequence_for<Types...>(),
                result,
                fields,
                reader
            );

            co_return result;
        }
    };
}
