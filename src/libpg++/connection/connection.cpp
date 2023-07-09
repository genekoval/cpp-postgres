#include <pg++/connection/connection.hpp>
#include <pg++/connection/type/byte.hpp>
#include <pg++/connection/type/error_fields.hpp>
#include <pg++/connection/type/int.hpp>
#include <pg++/connection/type/notification.hpp>
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
    connection::connection(
        netcore::socket&& socket,
        std::size_t buffer_size,
        notice_callback_type&& callback
    ) :
        notice_callback(std::forward<notice_callback_type>(callback)),
        socket(std::forward<netcore::socket>(socket), buffer_size)
    {}

    connection::~connection() {
        unlisten();
    }

    auto connection::authentication(std::string_view password) -> ext::task<> {
        const auto auth = co_await socket.read<std::int32_t>();

        TIMBER_TRACE("Authentication Code: {}", auth);

        switch (auth) {
            case 0:
                TIMBER_DEBUG("{} authentication was successful", *this);
                break;
            case 3:
                co_await password_message(password);
                break;
            default:
                throw error("authentication method not supported");
        }
    }

    auto connection::backend_key_data() -> ext::task<> {
        pid = co_await socket.read<std::int32_t>();
        secret = co_await socket.read<std::int32_t>();
    }

    auto connection::backend_pid() const noexcept -> std::int32_t {
        return pid;
    }

    auto connection::cancel() noexcept -> void {
        open = false;
        socket.cancel();
    }

    auto connection::find_channel(
        const std::string& name
    ) const noexcept -> std::shared_ptr<detail::channel> {
        const auto result = channels.find(name);

        if (result == channels.end()) return nullptr;
        return result->second.lock();
    }

    auto connection::close_portal(std::string_view name) -> ext::task<> {
        co_await socket.message('C', 'P', name);
    }

    auto connection::command_complete() -> ext::task<std::string> {
        co_return co_await socket.read<std::string>();
    }

    auto connection::consume_input() -> ext::task<detail::header> {
        co_await socket.flush();

        while (true) {
            const auto header = co_await socket.read<detail::header>();

            switch (header.code) {
                case 'A':
                    co_await notification_response();
                    break;
                case 'N':
                    co_await notice_response();
                    break;
                case 'S':
                    co_await parameter_status();
                    break;
                case 'E':
                    throw co_await error_response();
                default:
                    co_return header;
            }
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

    auto connection::describe(
        std::string_view portal
    ) -> ext::task<std::vector<column>> {
        co_await socket.message('D', 'P', portal);
        co_await flush();

        const auto header = co_await consume_input();

        switch (header.code) {
            case 'T':
                co_return co_await row_description();
            case 'n':
                co_return std::vector<column>();
            default:
                throw unexpected_message(header.code);
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
            const auto header = co_await consume_input();

            switch (header.code) {
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
                    throw unexpected_message(header.code);
            }
        } while (true);
    }

    auto connection::expect(
        std::string_view message,
        char code
    ) -> ext::task<std::int32_t> {
        const auto header = co_await consume_input();

        if (header.code != code) {
            throw error(fmt::format(
                "expected {}('{}'), received '{}'",
                message,
                code,
                header.code
            ));
        }

        co_return header.len;
    }

    auto connection::flush() -> ext::task<> {
        co_await socket.message('H');
    }

    auto connection::ignore(
        const std::unordered_set<std::int32_t>* ignored
    ) noexcept -> void {
        this->ignored = ignored;
    }

    auto connection::listen(
        const std::string& name,
        std::weak_ptr<detail::channel>&& channel
    ) -> void {
        channels.emplace(
            name,
            std::forward<std::weak_ptr<detail::channel>>(channel)
        );
    }

    auto connection::listeners(const std::string& channel) -> long {
        auto result = channels.find(channel);

        if (result == channels.end()) return 0;
        return result->second.use_count();
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

    auto connection::notification_response() -> ext::task<> {
        const auto notif = co_await socket.read<notification>();

        TIMBER_DEBUG(
            "{} asynchronous notification \"{}\" {}"
            "received from server process with PID {}",
            *this,
            notif.channel,
            notif.payload.empty() ? "" : fmt::format(
                R"(with payload "{}" )",
                notif.payload
            ),
            notif.pid
        );

        if (ignored && ignored->contains(notif.pid)) co_return;

        if (auto chan = find_channel(notif.channel)) {
            chan->notify(notif.payload, notif.pid);
            co_return;
        }

        TIMBER_WARNING(
            "{} no listeners registered for channel '{}'",
            *this,
            notif.channel
        );
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
    ) -> ext::task<> {
        co_await socket.message('P', name, query, types);
        co_await flush();
        co_await expect("ParseComplete", '1');
    }

    auto connection::password_message(
        std::string_view password
    ) -> ext::task<> {
        co_await socket.message('p', password);
        co_await socket.flush();
    }

    auto connection::ready_for_query() -> ext::task<> {
        const auto status = co_await socket.read<char>();
        this->status = static_cast<transaction_status>(status);
    }

    auto connection::row_description() -> ext::task<std::vector<column>> {
        co_return co_await socket.read<std::vector<column>>();
    }

    auto connection::simple_query(
        std::string_view query
    ) -> ext::task<std::vector<result>> {
        co_await socket.message('Q', query);

        auto tag = std::string();
        auto columns = std::vector<column>();
        auto rows = std::vector<row>();
        auto results = std::vector<result>();

        auto ready = false;

        do {
            const auto header = co_await consume_input();

            switch (header.code) {
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

    auto connection::startup_message(
        std::string_view password,
        const parameter_list& parameters
    ) -> ext::task<> {
        co_await socket.send(protocol::version, parameters, '\0');
        co_await socket.flush();

        auto ready = false;

        do {
            const auto res = co_await socket.read<detail::header>();

            switch (res.code) {
                case 'R':
                    co_await authentication(password);
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

        auto ready = false;

        do {
            const auto header = co_await consume_input();

            switch (header.code) {
                case '3':
                    TIMBER_DEBUG("Close Complete");
                    break;
                case 'Z':
                    co_await ready_for_query();
                    ready = true;
                    break;
            }
        } while (!ready);
    }

    auto connection::terminate() -> ext::task<> {
        co_await socket.message('X');
        TIMBER_TRACE("{} terminated connection", *this);
    }

    auto connection::unlisten() -> void {
        for (const auto& [name, channel] : channels) {
            if (auto chan = channel.lock()) chan->close();
        }
    }

    auto connection::unlisten(const std::string& channel) -> void {
        channels.erase(channel);
    }

    connection_handle::connection_handle(std::shared_ptr<mutex> conn) :
        conn(std::move(conn))
    {}

    connection_handle::~connection_handle() {
        if (conn) conn->get().cancel();
    }

    auto connection_handle::get() const noexcept -> connection& {
        return conn->get();
    }
    auto connection_handle::lock() -> ext::task<guard> {
        co_return co_await conn->lock();
    }

    auto connection_handle::shared() const noexcept -> std::shared_ptr<mutex> {
        return conn;
    }

    auto connection_handle::weak() const noexcept -> std::weak_ptr<mutex> {
        return conn;
    }
}
