#pragma once

#include "channel.hpp"
#include "socket.hpp"
#include "type/enum.hpp"
#include "type/int.hpp"
#include "type/parameter_list.hpp"
#include "type/sql.hpp"
#include "type/string.hpp"

#include <pg++/except/except.hpp>
#include <pg++/result/result.hpp>
#include <pg++/sql/type/int.hpp>
#include <pg++/sql/type/string.hpp>

namespace pg {
    enum class format : std::int16_t {
        text,
        binary
    };

    static_assert(detail::encodable<format>);

    using notice_callback_type = std::function<ext::task<>(notice&&)>;
}

namespace pg::detail {
    enum class transaction_status : char {
        // In a failed transaction block
        failure = 'E',

        // Not in a transaction block
        idle = 'I',

        // In a transaction block
        transaction = 'T'
    };

    class connection final {
        friend struct fmt::formatter<connection>;

        std::unordered_map<std::string, std::string> backend_parameters;

        std::unordered_map<std::string, std::weak_ptr<channel>> channels;

        const std::unordered_set<std::int32_t>* ignored = nullptr;

        // Newest minor protocol version supported by the server
        // for the major protocol version requested by the client.
        std::int32_t minor_version = 0;

        notice_callback_type notice_callback;

        bool open = false;

        // Process ID of this connection's backend.
        std::int32_t pid = 0;

        // Secret key of this connection's backend.
        std::int32_t secret = 0;

        detail::socket socket;

        // Current backend transaction status.
        transaction_status status;

        // Unsupported protocol options
        std::vector<std::string> unsupported_options;

        ext::continuation<header> continuation;

        ext::mutex can_read;

        auto authentication(std::string_view password) -> ext::task<>;

        auto backend_key_data() -> ext::task<>;

        auto command_complete() -> ext::task<std::string>;

        auto consume_input() -> ext::task<>;

        auto data_row(
            std::span<const column> columns
        ) -> ext::task<row>;

        template <typename T>
        requires pg::from_sql<T> || composite_type<T>
        auto data_row() -> ext::task<T> {
            const auto fields = co_await socket.read<std::int16_t>();

            if (fields > 1) {
                if constexpr (composite_type<T>) {
                    co_return co_await socket.from_row<T>(fields);
                }
                else {
                    throw unexpected_data("too many fields");
                }
            }

            co_return co_await socket.from_sql<T>();
        };

        auto describe(
            std::string_view portal
        ) -> ext::task<std::vector<column>>;

        auto error_response() -> ext::task<sql_error>;

        auto expect(
            std::string_view message,
            char code
        ) -> ext::task<std::int32_t>;

        template <typename R>
        auto extended_query(ext::task<R>&& task) -> ext::task<R> {
            auto result = R();
            auto exception = std::exception_ptr();

            try {
                result = co_await std::move(task);
            }
            catch (const sql_error&) {
                exception = std::current_exception();
            }

            co_await sync();

            if (exception) std::rethrow_exception(exception);
            co_return result;
        }

        auto negotiate_protocol_version() -> ext::task<>;

        auto notice_response() -> ext::task<>;

        auto notification_response() -> ext::task<>;

        auto parameter_status() -> ext::task<>;

        auto password_message(std::string_view password) -> ext::task<>;

        auto read_header() -> ext::task<std::pair<header, ext::mutex::guard>>;

        auto ready_for_query() -> ext::task<>;

        auto row_description() -> ext::task<std::vector<column>>;

        auto terminate() -> ext::task<>;
    public:
        connection() = default;

        connection(
            netcore::socket&& socket,
            std::size_t buffer_size,
            notice_callback_type&& callback = {}
        );

        ~connection();

        auto backend_pid() const noexcept -> std::int32_t;

        template <to_sql... Parameters>
        auto bind(
            std::string_view portal,
            std::string_view statement,
            format output,
            Parameters&&... parameters
        ) -> ext::task<> {
            co_await socket.message(
                'B',
                portal,
                statement,

                // Parameter format code
                std::int16_t(1),
                format::binary,

                std::int16_t(sizeof...(Parameters)),
                sql_type(std::forward<Parameters>(parameters))...,

                // Result format code
                std::int16_t(1),
                output
            );

            co_await flush();
            co_await expect("BindComplete", '2');
        }

        auto cancel() noexcept -> void;

        auto find_channel(
            const std::string& name
        ) const noexcept -> std::shared_ptr<detail::channel>;

        auto close_portal(std::string_view name) -> ext::task<>;

        template <pg::sql_type... Parameters>
        requires ((to_sql<Parameters> && ...) || (sizeof...(Parameters) == 0))
        auto exec(
            std::string_view query,
            Parameters&&... parameters
        ) -> ext::task<> {
            co_await prepare<Parameters...>("", query);
            co_await exec_prepared("", std::forward<Parameters>(parameters)...);
        }

        template <pg::sql_type... Parameters>
        requires ((to_sql<Parameters> && ...) || (sizeof...(Parameters) == 0))
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

        auto execute(
            std::string_view portal,
            std::span<const column> columns,
            std::vector<row>& rows,
            std::int32_t max_rows = 0 // fetch all rows by default
        ) -> ext::task<std::optional<std::string>>;

        template <typename T>
        requires composite_type<T> || pg::from_sql<T>
        auto execute(std::string_view portal) -> ext::task<T> {
            co_await socket.message('E', portal, 0);
            co_await flush();

            auto done = false;
            auto result = std::optional<T>();

            do {
                const auto [header, guard] = co_await read_header();

                switch (header.code) {
                    case 'D':
                        if (result) {
                            throw unexpected_data("too many rows returned");
                        }
                        result.emplace(co_await data_row<T>());
                        break;
                    case 'C':
                        co_await command_complete();
                        done = true;
                        break;
                    case 's':
                        throw unexpected_data("too many rows returned");
                    case 'I':
                        result.emplace();
                        break;
                    default:
                        throw unexpected_message(header.code);
                }
            } while (!done);

            if (!result) {
                throw unexpected_data("expected 1 row, received 0");
            }

            co_return std::move(*result);
        }

        template <typename Container>
        requires
            composite_type<typename Container::value_type> ||
            pg::from_sql<typename Container::value_type>
        auto execute(
            std::string_view portal,
            std::back_insert_iterator<Container> it,
            std::int32_t max_rows
        ) -> ext::task<std::optional<std::string>> {
            using Type = typename Container::value_type;

            co_await socket.message('E', portal, max_rows);
            co_await flush();

            do {
                const auto [header, guard] = co_await read_header();

                switch (header.code) {
                    case 'D':
                        it = co_await data_row<Type>();
                        break;
                    case 'C':
                        co_return co_await command_complete();
                    case 's':
                        co_return std::nullopt;
                    case 'I':
                        co_return std::string();
                    default:
                        throw unexpected_message(header.code);
                }
            } while (true);
        }

        template <typename Result, pg::sql_type... Parameters>
        requires
            (composite_type<Result> || pg::from_sql<Result>) &&
            ((to_sql<Parameters> && ...) || (sizeof...(Parameters) == 0))
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

        template <typename Result, pg::sql_type... Parameters>
        requires
            (composite_type<Result> || pg::from_sql<Result>) &&
            ((to_sql<Parameters> && ...) || (sizeof...(Parameters) == 0))
        auto fetch_prepared(
            std::string_view statement,
            Parameters&&... parameters
        ) -> ext::task<Result> {
            co_return co_await extended_query([&]() -> ext::task<Result> {
                co_await bind(
                    "",
                    statement,
                    format::binary,
                    std::forward<Parameters>(parameters)...
                );

                co_return co_await execute<Result>("");
            }());
        }

        template <typename T, pg::sql_type... Parameters>
        requires
            (composite_type<T> || pg::from_sql<T>) &&
            ((to_sql<Parameters> && ...) || (sizeof...(Parameters) == 0))
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

        template <typename T, pg::sql_type... Parameters>
        requires
            (composite_type<T> || pg::from_sql<T>) &&
            ((to_sql<Parameters> && ...) || (sizeof...(Parameters) == 0))
        auto fetch_rows_prepared(
            std::string_view statement,
            Parameters&&... parameters
        ) -> ext::task<std::vector<T>> {
            co_return co_await extended_query([&]() ->
                ext::task<std::vector<T>>
            {
                co_await bind(
                    "",
                    statement,
                    format::binary,
                    std::forward<Parameters>(parameters)...
                );

                auto rows = std::vector<T>();
                co_await execute("", std::back_inserter(rows), 0);
                co_return rows;
            }());
        }

        auto flush() -> ext::task<>;

        template <pg::sql_type T>
        auto get_oid() -> ext::task<std::int32_t> {
            using type = pg::type<std::remove_cvref_t<T>>;

            if constexpr (std::is_const_v<decltype(type::oid)>) {
                co_return type::oid;
            }
            else {
                if (type::oid != -1) co_return type::oid;

                if constexpr (pg::has_typname<T>) {
                    TIMBER_DEBUG("Reading OID for type '{}'", type::name);

                    const auto name = std::string_view { type::name };
                    const auto delimiter = name.find(".");

                    if (delimiter == std::string_view::npos) {
                        try {
                            type::oid = co_await fetch<std::int32_t>(
                                "SELECT oid FROM pg_type WHERE typname = $1",
                                type::name
                            );
                        }
                        catch (const unexpected_data&) {
                            throw unexpected_data(fmt::format(
                                "Failed to read OID: Type '{}' is ambiguous",
                                type::name
                            ));
                        }
                    }
                    else {
                        type::oid = co_await fetch<std::int32_t>(
                            "SELECT oid "
                            "FROM pg_type "
                            "WHERE "
                                "typname = $1 AND "
                                "typnamespace = ("
                                    "SELECT oid "
                                    "FROM pg_namespace "
                                    "WHERE nspname = $2"
                                ")",
                            name.substr(delimiter + 1),
                            name.substr(0, delimiter)
                        );
                    }

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

        auto ignore(
            const std::unordered_set<std::int32_t>* ignored
        ) noexcept -> void;

        auto listen(
            const std::string& name,
            std::weak_ptr<detail::channel>&& channel
        ) -> void;

        auto listeners(const std::string& channel) -> long;

        auto on_notice(notice_callback_type&& callback) -> void;

        auto parse(
            std::string_view name,
            std::string_view query,
            std::span<const std::int32_t> types
        ) -> ext::task<>;

        template <typename... Parameters>
        requires
            (pg::sql_type<Parameters> && ...) ||
            (sizeof...(Parameters) == 0)
        auto prepare(
            std::string_view name,
            std::string_view query
        ) -> ext::task<> {
            const auto types = std::array<std::int32_t, sizeof...(Parameters)> {
                co_await get_oid<Parameters>()...
            };

            auto exception = std::exception_ptr();
            try {
                co_await parse(name, query, types);
            }
            catch (const sql_error&) {
                exception = std::current_exception();
            }

            if (exception) {
                co_await sync();
                std::rethrow_exception(exception);
            }

            TIMBER_DEBUG(
                "{} prepare {}: {}",
                *this,
                name.empty() ? "unnamed statement" : fmt::format(
                    "statement '{}'",
                    name
                ),
                query
            );
        }

        template <typename T, typename... Args>
        requires (pg::sql_type<Args> && ...) || (sizeof...(Args) == 0)
        auto prepare_fn(
            std::string_view name,
            auto (T::*)(Args...)
        ) -> ext::task<> {
            constexpr auto arg_count = sizeof...(Args);

            auto os = std::ostringstream();

            os << "SELECT * FROM " << name << "(";

            for (std::size_t i = 1; i <= arg_count; ++i) {
                os << "$" << i;

                if (i < arg_count) os << ", ";
            }

            os << ")";

            co_await prepare<Args...>(name, os.str());
        }

        template <pg::sql_type... Parameters>
        requires (to_sql<Parameters> && ...) || (sizeof...(Parameters) == 0)
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

        template <pg::sql_type... Parameters>
        requires (to_sql<Parameters> && ...) || (sizeof...(Parameters) == 0)
        auto query_prepared(
            std::string_view statement,
            Parameters&&... parameters
        ) -> ext::task<result> {
            co_return co_await extended_query([&]() -> ext::task<result> {
                co_await bind(
                    "", // unnamed portal
                    statement,
                    format::text,
                    std::forward<Parameters>(parameters)...
                );

                auto columns = co_await describe("");
                auto rows = std::vector<row>();

                const auto tag = co_await execute(
                    "",
                    columns,
                    rows
                );

                co_return result { *tag, std::move(columns), std::move(rows) };
            }());
        }

        auto simple_query(
            std::string_view query
        ) -> ext::task<std::vector<result>>;

        auto startup_message(
            std::string_view password,
            const parameter_list& parameters
        ) -> ext::task<>;

        auto sync() -> ext::task<>;

        auto unlisten() -> void;

        auto unlisten(const std::string& channel) -> void;

        auto wait_for_input() -> ext::task<>;
    };

    struct connection_handle final {
        using mutex = netcore::mutex<connection>;
        using guard = mutex::guard;
    private:
        std::shared_ptr<mutex> conn;
    public:
        connection_handle() = default;

        connection_handle(std::shared_ptr<mutex> conn);

        connection_handle(const connection_handle&) = delete;

        connection_handle(connection_handle&& other) = default;

        ~connection_handle();

        auto operator=(const connection_handle&) -> connection_handle& = delete;

        auto operator=(connection_handle&&) -> connection_handle& = default;

        auto get() const noexcept -> connection&;

        auto lock() -> ext::task<guard>;

        auto shared() const noexcept -> std::shared_ptr<mutex>;

        auto weak() const noexcept -> std::weak_ptr<mutex>;
    };

    auto run_connection_task(
        std::shared_ptr<connection_handle::mutex> conn
    ) -> ext::detached_task;
}

template <>
struct fmt::formatter<pg::detail::connection> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(
        const pg::detail::connection& connection,
        FormatContext& ctx
    ) {
        return format_to(
            ctx.out(),
            "postgres{}",
            connection.pid == 0 ? "" : fmt::format("[{}]", connection.pid)
        );
    }
};
