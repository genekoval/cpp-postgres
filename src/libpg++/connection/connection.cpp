#include <pg++/connection/connection.hpp>
#include <pg++/connection/type/byte.hpp>
#include <pg++/connection/type/error_fields.hpp>
#include <pg++/connection/type/int.hpp>
#include <pg++/connection/type/parameter_list.hpp>
#include <pg++/connection/type/span.hpp>
#include <pg++/connection/type/vector.hpp>
#include <pg++/except/except.hpp>

namespace {
    namespace protocol {
        constexpr std::uint32_t major = 3;
        constexpr std::uint32_t minor = 0;

        constexpr std::int32_t version =
            static_cast<std::int32_t>(major << 16 | minor);
    }
}

namespace pg::detail {
    auto awaitable::await_ready() const noexcept -> bool {
        return false;
    }

    auto awaitable::await_suspend(std::coroutine_handle<> coroutine) -> void {
        this->coroutine = coroutine;
    }

    auto awaitable::await_resume() const -> header {
        ref.get().reset();
        if (exception) std::rethrow_exception(exception);
        return response;
    }

    connection::connection(
        netcore::socket&& socket,
        notice_callback_type&& callback
    ) :
        notice_callback(std::forward<notice_callback_type>(callback)),
        socket(std::forward<netcore::socket>(socket))
    {}

    auto connection::authentication() -> ext::task<> {
        const auto auth = co_await socket.read<std::int32_t>();

        TIMBER_TRACE("Authentication Code: {}", auth);

        switch (auth) {
            case 0:
                // authentication was successful
                break;
            default:
                throw error("authentication method not supported");
        }
    }

    auto connection::backend_key_data() -> ext::task<> {
        pid = co_await socket.read<std::int32_t>();
        secret = co_await socket.read<std::int32_t>();
    }


    auto connection::cancel() noexcept -> void {
        open = false;
        socket.cancel();
    }

    auto connection::close_portal(std::string_view name) -> ext::task<> {
        co_await socket.message('C', 'P', name);
        co_await expect("CloseComplete", '3');

        TIMBER_TRACE(R"(Portal "{}" closed)", name);
    }

    auto connection::command_complete() -> ext::task<std::string> {
        co_return co_await socket.read<std::string>();
    }

    auto connection::consume_input() -> ext::task<> {
        const auto res = co_await socket.read<header>();

        switch (res.code) {
            case 'N':
                co_await notice_response();
                break;
            case 'S':
                co_await parameter_status();
                break;
            case 'E':
                {
                    auto error = co_await error_response();

                    if (waiter) {
                        auto& waiter = this->waiter->get();
                        waiter.exception = std::make_exception_ptr(error);
                        waiter.coroutine.resume();
                    }
                    else throw std::move(error);
                }
                break;
            default:
                if (waiter) {
                    auto& waiter = this->waiter->get();
                    waiter.response = res;
                    waiter.coroutine.resume();
                }
                else throw error(fmt::format(
                    "received message byte without an active handler: '{}'",
                    res.code
                ));
                break;
        }
    }

    auto connection::data_row(
        std::span<const column> columns
    ) -> ext::task<row> {
        const auto size = co_await socket.read<std::int16_t>();
        auto fields = std::vector<field>();
        fields.reserve(size);

        for (auto i = 0; i < size; ++i) {
            auto data = co_await socket.read<field_data>();
            fields.emplace_back(columns[i], std::move(data));
        }

        co_return row(columns, std::move(fields));
    }

    auto connection::defer(std::string_view message, char code) -> ext::task<> {
        co_await expect(message, code);
    }

    auto connection::describe(
        std::string_view portal
    ) -> ext::task<ext::task<std::vector<column>>> {
        co_await socket.message('D', 'P', portal);
        co_return describe_complete();
    }

    auto connection::describe_complete() -> ext::task<std::vector<column>> {
        const auto res = co_await read_header();

        switch (res.code) {
            case 'T':
                co_return co_await row_description();
            case 'n':
                co_return std::vector<column>();
            default:
                throw error(fmt::format(
                    "received unexpected message byte '{}'",
                    res.code
                ));
        }
    }

    auto connection::error_response() -> ext::task<sql_error> {
        co_return sql_error(co_await socket.read<error_fields>());
    }

    auto connection::execute(
        std::string_view portal,
        std::span<const column> columns,
        std::vector<row>& rows,
        std::int32_t max_rows
    ) -> ext::task<std::optional<std::string>> {
        rows.clear();

        co_await socket.message('E', portal, max_rows);
        co_await flush();

        do {
            const auto res = co_await read_header();

            switch (res.code) {
                case 'D':
                    rows.emplace_back(co_await data_row(columns));
                    break;
                case 'C':
                    co_return co_await command_complete();
                case 's':
                    // Portal Suspended
                    co_return std::nullopt;
                case 'I':
                    // Empty Query Response
                    co_return std::string();
                default:
                    throw error(fmt::format(
                        "received unexpected message byte '{}'",
                        res.code
                    ));
            }
        } while (true);
    }

    auto connection::expect(
        std::string_view message,
        char code
    ) -> ext::task<std::int32_t> {
        const auto res = co_await read_header();

        if (res.code != code) {
            throw error(fmt::format(
                "expected {}('{}'), received '{}'",
                message,
                code,
                res.code
            ));
        }

        co_return res.len;
    }

    auto connection::flush() -> ext::task<> {
        co_await socket.message('H');
    }

    auto connection::negotiate_protocol_version() -> ext::task<> {
        minor_version = co_await socket.read<std::int32_t>();

        const auto size = co_await socket.read<std::int32_t>();

        for (auto i = 0; i < size; ++i) {
            unsupported_options.push_back(co_await socket.read<std::string>());
        }
    }

    auto connection::notice_response() -> ext::task<> {
        auto notice = co_await socket.read<pg::notice>();
        TIMBER_DEBUG("{} {}: {}", *this, notice.severity, notice.message);
        if (notice_callback) co_await notice_callback(std::move(notice));
    }

    auto connection::on_notice(notice_callback_type&& callback) -> void {
        notice_callback = std::forward<notice_callback_type>(callback);
    }

    auto connection::parameter_status() -> ext::task<> {
        const auto key = co_await socket.read<std::string>();
        const auto value = co_await socket.read<std::string>();

        TIMBER_TRACE(R"({} parameter: {} = "{}")", *this, key, value);

        backend_parameters[key] = value;
    }

    auto connection::parse(
        std::string_view name,
        std::string_view query,
        std::span<const std::int32_t> types
    ) -> ext::task<ext::task<>> {
        co_await socket.message('P', name, query, types);
        co_return defer("ParseComplete", '1');
    }

    auto connection::query(
        std::string_view query
    ) -> ext::task<std::vector<result>> {
        co_await socket.message('Q', query);

        auto tag = std::string();
        auto columns = std::vector<column>();
        auto rows = std::vector<row>();
        auto results = std::vector<result>();

        auto ready = false;

        do {
            const auto res = co_await read_header();

            switch (res.code) {
                case 'T':
                    columns = co_await row_description();
                    break;
                case 'D':
                    rows.emplace_back(co_await data_row(columns));
                    break;
                case 'C':
                    tag = co_await command_complete();
                    results.emplace_back(
                        tag,
                        std::exchange(columns, {}),
                        std::exchange(rows, {})
                    );
                    break;
                case 'I':
                    // Empty Query Response
                    results.emplace_back();
                    break;
                case 'Z':
                    co_await ready_for_query();
                    ready = true;
                    break;
            }
        } while (!ready);

        co_return results;
    }

    auto connection::read_header() -> ext::task<header> {
        if (!open) throw broken_connection();

        co_await socket.flush();

        auto waiter = awaitable { .ref = this->waiter };
        this->waiter = waiter;
        co_return co_await waiter;
    }

    auto connection::ready_for_query() -> ext::task<> {
        const auto status = co_await socket.read<char>();
        this->status = static_cast<transaction_status>(status);
    }

    auto connection::row_description() -> ext::task<std::vector<column>> {
        co_return co_await socket.read<std::vector<column>>();
    }

    auto connection::startup_message(
        const parameter_list& parameters
    ) -> ext::task<> {
        co_await socket.send(protocol::version, parameters, '\0');
        co_await socket.flush();

        auto ready = false;

        do {
            const auto res = co_await socket.read<header>();

            switch (res.code) {
                case 'R':
                    co_await authentication();
                    break;
                case 'v':
                    co_await negotiate_protocol_version();
                    break;
                case 'K':
                    co_await backend_key_data();
                    break;
                case 'S':
                    co_await parameter_status();
                    break;
                case 'Z':
                    co_await ready_for_query();
                    ready = true;
                    break;
                case 'E':
                    throw co_await error_response();
                case 'N':
                    co_await notice_response();
                    break;
            }
        } while (!ready);

        TIMBER_DEBUG("{} connected to {}", socket, *this);
    }

    auto connection::sync() -> ext::task<> {
        co_await socket.message('S');
        co_await expect("ReadyForQuery", 'Z');
        co_await ready_for_query();
    }

    auto connection::terminate() -> ext::task<> {
        co_await socket.message('X');
        TIMBER_TRACE("{} terminated connection", *this);
    }

    auto connection::wait_for_input() -> ext::task<> {
        open = true;

        try {
            do {
                try {
                    co_await consume_input();
                }
                catch (const netcore::task_canceled&) {
                    open = false;
                }
            } while (open);

            co_await terminate();
        }
        catch (const std::exception& ex) {
            TIMBER_ERROR("{} connection aborted: {}", *this, ex.what());
        }
        catch (...) {
            TIMBER_ERROR("{} connection aborted", *this);
        }

        open = false;
    }

    connection_handle::connection_handle(std::shared_ptr<connection>&& conn) :
        conn(std::forward<std::shared_ptr<connection>>(conn))
    {}

    connection_handle::~connection_handle() {
        if (conn) conn->cancel();
    }

    auto connection_handle::operator*() const noexcept -> connection& {
        return *conn;
    }

    auto connection_handle::operator->() const noexcept -> connection* {
        return conn.get();
    }

    auto run_connection_task(
        std::shared_ptr<connection>& conn
    ) -> ext::detached_task {
        auto copy = conn;
        co_await copy->wait_for_input();
    }
}
