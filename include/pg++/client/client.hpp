#pragma once

#include "portal.hpp"
#include "transaction.hpp"

namespace pg {
    class client final {
        detail::connection_handle handle;
    public:
        client() = default;

        explicit client(
            std::shared_ptr<netcore::mutex<detail::connection>> connection
        );

        auto backend_pid() const noexcept -> std::int32_t;

        auto begin() -> ext::task<transaction>;

        template <sql_type... Parameters>
        requires((to_sql<Parameters> && ...) || (sizeof...(Parameters) == 0))
        auto exec(std::string_view query, Parameters&&... parameters)
            -> ext::task<> {
            auto connection = co_await handle.lock();
            co_await connection->exec(
                query,
                std::forward<Parameters>(parameters)...
            );
        }

        template <sql_type... Parameters>
        requires((to_sql<Parameters> && ...) || (sizeof...(Parameters) == 0))
        auto exec_prepared(
            std::string_view statement,
            Parameters&&... parameters
        ) -> ext::task<> {
            auto connection = co_await handle.lock();
            co_await connection->exec_prepared(
                statement,
                std::forward<Parameters>(parameters)...
            );
        }

        template <typename Result, sql_type... Parameters>
        requires(composite_type<Result> || from_sql<Result>) &&
                ((to_sql<Parameters> && ...) || (sizeof...(Parameters) == 0))
        auto fetch(std::string_view query, Parameters&&... parameters)
            -> ext::task<Result> {
            auto connection = co_await handle.lock();
            co_return co_await connection->template fetch<Result>(
                query,
                std::forward<Parameters>(parameters)...
            );
        }

        template <typename Result, sql_type... Parameters>
        requires(composite_type<Result> || from_sql<Result>) &&
                ((to_sql<Parameters> && ...) || (sizeof...(Parameters) == 0))
        auto fetch_prepared(
            std::string_view statement,
            Parameters&&... parameters
        ) -> ext::task<Result> {
            auto connection = co_await handle.lock();
            co_return co_await connection->template fetch_prepared<Result>(
                statement,
                std::forward<Parameters>(parameters)...
            );
        }

        template <typename T, sql_type... Parameters>
        requires(composite_type<T> || from_sql<T>) &&
                ((to_sql<Parameters> && ...) || (sizeof...(Parameters) == 0))
        auto fetch_rows(std::string_view query, Parameters&&... parameters)
            -> ext::task<std::vector<T>> {
            auto connection = co_await handle.lock();
            co_return co_await connection->template fetch_rows<T>(
                query,
                std::forward<Parameters>(parameters)...
            );
        }

        template <typename T, sql_type... Parameters>
        requires(composite_type<T> || from_sql<T>) &&
                ((to_sql<Parameters> && ...) || (sizeof...(Parameters) == 0))
        auto fetch_rows_prepared(
            std::string_view statement,
            Parameters&&... parameters
        ) -> ext::task<std::vector<T>> {
            auto connection = co_await handle.lock();
            co_return co_await connection->template fetch_rows_prepared<T>(
                statement,
                std::forward<Parameters>(parameters)...
            );
        }

        auto ignore(const std::unordered_set<std::int32_t>& ignored) noexcept
            -> void;

        auto ignore(const std::string& channel, std::int32_t pid) noexcept
            -> void;

        auto listen(const std::string& channel) -> ext::task<pg::channel>;

        auto listeners(const std::string& channel) -> long;

        auto on_notice(notice_callback_type&& callback) -> void;

        template <typename... Parameters>
        requires(sql_type<Parameters> && ...) || (sizeof...(Parameters) == 0)
        auto prepare(std::string_view name, std::string_view query)
            -> ext::task<> {
            auto connection = co_await handle.lock();
            co_await connection->template prepare<Parameters...>(name, query);
        }

        template <typename T, typename... Args>
        requires(sql_type<Args> && ...) || (sizeof...(Args) == 0)
        auto prepare_fn(std::string_view name, auto (T::*fn)(Args...))
            -> ext::task<> {
            auto connection = co_await handle.lock();
            co_await connection->prepare_fn(name, fn);
        }

        template <sql_type... Parameters>
        requires(to_sql<Parameters> && ...) || (sizeof...(Parameters) == 0)
        auto query(std::string_view query, Parameters&&... parameters)
            -> ext::task<result> {
            auto connection = co_await handle.lock();
            co_return co_await connection->query(
                query,
                std::forward<Parameters>(parameters)...
            );
        }

        template <sql_type... Parameters>
        requires(to_sql<Parameters> && ...) || (sizeof...(Parameters) == 0)
        auto query_prepared(
            std::string_view statement,
            Parameters&&... parameters
        ) -> ext::task<result> {
            auto connection = co_await handle.lock();
            co_return co_await connection->query_prepared(
                statement,
                std::forward<Parameters>(parameters)...
            );
        }

        auto simple_query(std::string_view query)
            -> ext::task<std::vector<result>>;

        template <typename T, sql_type... Parameters>
        requires(composite_type<T> || from_sql<T>) &&
                ((to_sql<Parameters> && ...) || (sizeof...(Parameters) == 0))
        auto stream(
            std::string_view name,
            std::string_view query,
            std::int32_t max_rows,
            Parameters&&... parameters
        ) -> ext::task<portal<T>> {
            co_await prepare<Parameters...>("", query);
            co_return co_await stream_prepared<T>(
                name,
                "",
                max_rows,
                std::forward<Parameters>(parameters)...
            );
        }

        template <typename T, sql_type... Parameters>
        requires(composite_type<T> || from_sql<T>) &&
                ((to_sql<Parameters> && ...) || (sizeof...(Parameters) == 0))
        auto stream_prepared(
            std::string_view name,
            std::string_view statement,
            std::int32_t max_rows,
            Parameters&&... parameters
        ) -> ext::task<portal<T>> {
            auto connection = co_await handle.lock();
            co_await connection->bind(
                name,
                statement,
                format::binary,
                std::forward<Parameters>(parameters)...
            );

            co_return portal<T>(handle.shared(), name, max_rows);
        }

        template <typename Result, sql_type... Parameters>
        requires(composite_type<Result> || from_sql<Result>) &&
                ((to_sql<Parameters> && ...) || (sizeof...(Parameters) == 0))
        auto try_fetch(std::string_view query, Parameters&&... parameters)
            -> ext::task<std::optional<Result>> {
            auto connection = co_await handle.lock();
            co_return co_await connection->template try_fetch<Result>(
                query,
                std::forward<Parameters>(parameters)...
            );
        }

        template <typename Result, sql_type... Parameters>
        requires(composite_type<Result> || from_sql<Result>) &&
                ((to_sql<Parameters> && ...) || (sizeof...(Parameters) == 0))
        auto try_fetch_prepared(
            std::string_view statement,
            Parameters&&... parameters
        ) -> ext::task<std::optional<Result>> {
            auto connection = co_await handle.lock();
            co_return co_await connection->template try_fetch_prepared<Result>(
                statement,
                std::forward<Parameters>(parameters)...
            );
        }

        auto unignore() noexcept -> void;

        auto unignore(const std::string& channel, std::int32_t pid) noexcept
            -> void;

        auto unlisten() -> ext::task<>;

        auto unlisten(const std::string& channel) -> ext::task<>;
    };
}
