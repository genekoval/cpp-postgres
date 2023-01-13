#pragma once

#include <pg++/connection/connection.hpp>

namespace pg {
    class client final {
        static auto make_function_query(
            std::string_view function,
            int arg_count
        ) -> std::string;

        detail::connection_handle connection;

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

        template <sql_type... Parameters>
        requires ((to_sql<Parameters>, ...) || (sizeof...(Parameters) == 0))
        auto exec(
            std::string_view query,
            Parameters&&... parameters
        ) -> ext::task<> {
            co_await prepare<Parameters...>("", query);
            co_await exec_prepared("", std::forward<Parameters>(parameters)...);
        }

        template <sql_type... Parameters>
        requires ((to_sql<Parameters>, ...) || (sizeof...(Parameters) == 0))
        auto exec_prepared(
            std::string_view statement,
            Parameters&&... parameters
        ) -> ext::task<> {
            const auto result = co_await query_prepared(
                statement,
                std::forward<Parameters>(parameters)...
            );

            if (!result.empty()) throw unexpected_data(fmt::format(
                "expected zero rows, received {}",
                result.size()
            ));
        }

        template <typename Result, sql_type... Parameters>
        requires
            (composite_type<Result> || from_sql<Result>) &&
            ((to_sql<Parameters>, ...) || (sizeof...(Parameters) == 0))
        auto fetch(
            std::string_view query,
            Parameters&&... parameters
        ) -> ext::task<Result> {
            co_await prepare<Parameters...>("", query);
            co_return co_await fetch_prepared<Result>(
                "",
                std::forward<Parameters>(parameters)...
            );
        }

        template <typename Result, sql_type... Parameters>
        requires
            (composite_type<Result> || from_sql<Result>) &&
            ((to_sql<Parameters>, ...) || (sizeof...(Parameters) == 0))
        auto fetch_prepared(
            std::string_view statement,
            Parameters&&... parameters
        ) -> ext::task<Result> {
            const auto bind = co_await connection->bind(
                "",
                statement,
                format::binary,
                std::forward<Parameters>(parameters)...
            );

            co_await connection->flush();
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
            co_await prepare<Parameters...>("", query);
            co_return co_await fetch_rows_prepared<T>(
                "",
                std::forward<Parameters>(parameters)...
            );
        }

        template <typename T, sql_type... Parameters>
        requires
            (composite_type<T> || from_sql<T>) &&
            ((to_sql<Parameters>, ...) || (sizeof...(Parameters) == 0))
        auto fetch_rows_prepared(
            std::string_view statement,
            Parameters&&... parameters
        ) -> ext::task<std::vector<T>> {
            const auto bind = co_await connection->bind(
                "",
                statement,
                format::binary,
                std::forward<Parameters>(parameters)...
            );

            co_await connection->flush();
            co_await bind;

            auto rows = std::vector<T>();
            co_await connection->execute("", std::back_inserter(rows), 0);

            co_await connection->sync();
            co_return rows;
        }

        template <sql_type T>
        auto get_oid() -> ext::task<std::int32_t> {
            using type = pg::type<std::remove_cvref_t<T>>;

            if constexpr (std::is_const_v<decltype(type::oid)>) {
                co_return type::oid;
            }
            else {
                if (type::oid != -1) co_return type::oid;

                if constexpr (pg::has_typname<T>) {
                    TIMBER_DEBUG("Reading OID for type '{}'", type::name);

                    type::oid = co_await fetch<std::int32_t>(
                        "SELECT oid FROM pg_type WHERE typname = $1",
                        type::name
                    );

                    TIMBER_DEBUG(
                        "OID for type '{}' is {}",
                        type::name,
                        type::oid
                    );
                }
                else if (pg::has_oid_query<T>) {
                    co_await type::oid_query(*this);
                }

                co_return type::oid;
            }
        }

        auto on_notice(notice_callback_type&& callback) -> void;

        template <typename... Parameters>
        requires (sql_type<Parameters> && ...) || (sizeof...(Parameters) == 0)
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

            TIMBER_DEBUG("Prepare statement '{}': {}", name, query);
        }

        template <typename T, typename... Args>
        requires (sql_type<Args> && ...) || (sizeof...(Args) == 0)
        auto prepare_fn(
            std::string_view name,
            auto (T::*)(Args...)
        ) -> ext::task<> {
            const auto query = make_function_query(name, sizeof...(Args));
            co_await prepare<Args...>(name, query);
        }

        template <sql_type... Parameters>
        requires (to_sql<Parameters>, ...) || (sizeof...(Parameters) == 0)
        auto query(
            std::string_view query,
            Parameters&&... parameters
        ) -> ext::task<result> {
            co_await prepare<Parameters...>("", query);
            co_return co_await query_prepared(
                "",
                std::forward<Parameters>(parameters)...
            );
        }

        template <sql_type... Parameters>
        requires (to_sql<Parameters>, ...) || (sizeof...(Parameters) == 0)
        auto query_prepared(
            std::string_view statement,
            Parameters&&... parameters
        ) -> ext::task<result> {
            const auto bind = co_await connection->bind(
                "", // unnamed portal
                statement,
                format::text,
                std::forward<Parameters>(parameters)...
            );

            const auto describe = co_await connection->describe("");

            co_await connection->flush();
            co_await bind;

            auto columns = co_await describe;
            auto rows = std::vector<row>();

            const auto tag = co_await connection->execute(
                "",
                columns,
                rows
            );

            co_await connection->sync();

            co_return result { *tag, std::move(columns), std::move(rows) };
        }

        auto simple_query(
            std::string_view query
        ) -> ext::task<std::vector<result>>;
    };
}
