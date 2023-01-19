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

    struct awaitable final {
        std::coroutine_handle<> coroutine;
        std::exception_ptr exception;
        header response;
        std::reference_wrapper<
            std::optional<std::reference_wrapper<awaitable>>
        > ref;

        auto await_ready() const noexcept -> bool;

        auto await_suspend(std::coroutine_handle<> coroutine) -> void;

        auto await_resume() const -> header;
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

        socket socket;

        // Current backend transaction status.
        transaction_status status;

        // Unsupported protocol options
        std::vector<std::string> unsupported_options;

        std::optional<std::reference_wrapper<awaitable>> waiter;

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

        auto defer(std::string_view message, char code) -> ext::task<>;

        auto describe_complete() -> ext::task<std::vector<column>>;

        auto error_response() -> ext::task<sql_error>;

        auto expect(
            std::string_view message,
            char code
        ) -> ext::task<std::int32_t>;

        auto negotiate_protocol_version() -> ext::task<>;

        auto notice_response() -> ext::task<>;

        auto notification_response() -> ext::task<>;

        auto parameter_status() -> ext::task<>;

        auto password_message(std::string_view password) -> ext::task<>;

        auto read_header() -> ext::task<header>;

        auto ready_for_query() -> ext::task<>;

        auto row_description() -> ext::task<std::vector<column>>;

        auto terminate() -> ext::task<>;
    public:
        connection() = default;

        connection(
            netcore::socket&& socket,
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
        ) -> ext::task<ext::task<>> {
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

            co_return defer("BindComplete", '2');
        }

        auto cancel() noexcept -> void;

        auto channel(
            const std::string& name
        ) const noexcept -> std::shared_ptr<channel>;

        auto close_portal(std::string_view name) -> ext::task<>;

        auto describe(
            std::string_view portal
        ) -> ext::task<ext::task<std::vector<column>>>;

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
                const auto res = co_await read_header();

                switch (res.code) {
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
                        throw error(fmt::format(
                            "received unexpected message byte '{}'",
                            res.code
                        ));
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
                const auto res = co_await read_header();

                switch (res.code) {
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
                        throw error(fmt::format(
                            "received unexpected message byte '{}'",
                            res.code
                        ));
                }
            } while (true);
        }

        auto flush() -> ext::task<>;

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
        ) -> ext::task<ext::task<>>;

        auto query(std::string_view query) -> ext::task<std::vector<result>>;

        auto startup_message(
            std::string_view password,
            const parameter_list& parameters
        ) -> ext::task<>;

        auto sync() -> ext::task<>;

        auto unlisten() -> void;

        auto unlisten(const std::string& channel) -> void;

        auto wait_for_input() -> ext::task<>;
    };

    class connection_handle final {
        std::shared_ptr<connection> conn;
    public:
        connection_handle() = default;

        connection_handle(std::shared_ptr<connection>&& conn);

        connection_handle(const connection_handle&) = delete;

        connection_handle(connection_handle&& other) = default;

        ~connection_handle();

        auto operator=(const connection_handle&) -> connection_handle& = delete;

        auto operator=(connection_handle&&) -> connection_handle& = default;

        auto operator*() const noexcept -> connection&;

        auto operator->() const noexcept -> connection*;

        auto weak() const noexcept -> std::weak_ptr<connection>;
    };

    auto run_connection_task(
        std::shared_ptr<connection>& conn
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
