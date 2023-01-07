#pragma once

#include <pg++/connection/connection.hpp>

namespace pg {
    class client final {
        detail::connection_handle connection;

        template <sql_type T>
        auto get_oid() -> ext::task<std::int32_t> {
            using type = pg::type<std::remove_cvref_t<T>>;

            if constexpr (std::is_const_v<decltype(type::oid)>) {
                co_return type::oid;
            }
            else {
                if (type::oid != -1) co_return type::oid;

                TIMBER_DEBUG("Reading OID for type '{}'", type::name);

                type::oid = co_await fetch<std::int32_t>(
                    "SELECT oid FROM pg_type WHERE typname = $1",
                    type::name
                );

                TIMBER_DEBUG("OID for type '{}' is {}", type::name, type::oid);

                co_return type::oid;
            }
        }

        template <sql_type... Parameters>
        auto deferred_prepare(
            std::string_view name,
            std::string_view query
        ) -> ext::task<ext::task<>> {
            const auto types = std::array<std::int32_t, sizeof...(Parameters)> {
                co_await get_oid<Parameters>()...
            };

            co_return co_await connection->parse(
                name,
                query,
                types
            );
        }
    public:
        client() = default;

        client(std::shared_ptr<detail::connection>&& connection);

        template <typename Result, sql_type... Parameters>
        requires
            (composite_type<Result> || from_sql<Result>) &&
            ((to_sql<Parameters>, ...) || (sizeof...(Parameters) == 0))
        auto fetch(
            std::string_view query,
            Parameters&&... parameters
        ) -> ext::task<Result> {
            const auto parse = co_await deferred_prepare<Parameters...>(
                "",
                query
            );

            const auto bind = co_await connection->bind(
                "",
                "",
                format::binary,
                std::forward<Parameters>(parameters)...
            );

            co_await connection->flush();
            co_await parse;
            co_await bind;

            auto result = co_await connection->execute<Result>("");

            co_await connection->sync();
            co_return result;
        }

        template <typename T, sql_type... Parameters>
        requires
            (composite_type<T> || from_sql<T>) &&
            ((to_sql<Parameters>, ...) || (sizeof...(Parameters) == 0))
        auto fetch_rows(
            std::string_view query,
            Parameters&&... parameters
        ) -> ext::task<std::vector<T>> {
            const auto parse = co_await deferred_prepare<Parameters...>(
                "",
                query
            );

            const auto bind = co_await connection->bind(
                "",
                "",
                format::binary,
                std::forward<Parameters>(parameters)...
            );

            co_await connection->flush();
            co_await parse;
            co_await bind;

            auto rows = std::vector<T>();
            co_await connection->execute("", std::back_inserter(rows), 0);

            co_await connection->sync();
            co_return rows;
        }

        auto on_notice(notice_callback_type&& callback) -> void;

        template <sql_type... Parameters>
        auto prepare(
            std::string_view name,
            std::string_view query
        ) -> ext::task<> {
            const auto parse = co_await deferred_prepare<Parameters...>(
                name,
                query
            );

            co_await connection->flush();
            co_await parse;
        }

        template <sql_type... Parameters>
        requires (to_sql<Parameters>, ...) || (sizeof...(Parameters) == 0)
        auto query(
            std::string_view query,
            Parameters&&... parameters
        ) -> ext::task<result> {
            const auto parse = co_await deferred_prepare<Parameters...>(
                "", // unnamed prepared statement
                query
            );

            const auto bind = co_await connection->bind(
                "", // unnamed portal
                "", // unnamed prepared statement
                format::text,
                std::forward<Parameters>(parameters)...
            );

            const auto describe = co_await connection->describe("");

            co_await connection->flush();
            co_await parse;
            co_await bind;

            auto columns = co_await describe;
            auto rows = std::vector<row>();

            const auto tag = co_await connection->execute(
                "",
                columns,
                rows
            );

            co_await connection->sync();

            co_return result(*tag, std::move(columns), std::move(rows));
        }

        auto simple_query(
            std::string_view query
        ) -> ext::task<std::vector<result>>;
    };
}
