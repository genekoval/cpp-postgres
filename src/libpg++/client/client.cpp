#include <pg++/client/client.hpp>

namespace pg {
    client::client(std::shared_ptr<detail::connection>&& connection) :
        connection(
            std::forward<std::shared_ptr<detail::connection>>(connection)
        )
    {}

    auto client::backend_pid() const noexcept -> std::int32_t {
        return connection->backend_pid();
    }

    auto client::ignore(
        const std::unordered_set<std::int32_t>& ignored
    ) noexcept -> void {
        connection->ignore(&ignored);
    }

    auto client::ignore(
        const std::string& channel,
        std::int32_t pid
    ) noexcept -> void {
        auto chan = connection->channel(channel);
        if (chan) chan->ignore(pid);
    }

    auto client::listen(const std::string& channel) -> ext::task<pg::channel> {
        auto chan = connection->channel(channel);
        if (chan) co_return chan;

        co_await exec(fmt::format("LISTEN {}", channel));

        chan = std::shared_ptr<detail::channel>(
            new detail::channel(channel, connection.weak())
        );

        connection->listen(channel, chan);
        co_return chan;
    }

    auto client::listeners(const std::string& channel) -> long {
        return connection->listeners(channel);
    }

    auto client::make_function_query(
        std::string_view function,
        int arg_count
    ) -> std::string {
        auto os = std::ostringstream();

        os << "SELECT * FROM " << function << "(";

        for (auto i = 1; i <= arg_count; ++i) {
            os << "$" << i;

            if (i < arg_count) os << ", ";
        }

        os << ")";

        return os.str();
    }

    auto client::on_notice(notice_callback_type&& callback) -> void {
        connection->on_notice(std::forward<notice_callback_type>(callback));
    }

    auto client::simple_query(
        std::string_view query
    ) -> ext::task<std::vector<result>> {
        return connection->query(query);
    }

    auto client::unignore() noexcept -> void {
        connection->ignore(nullptr);
    }

    auto client::unignore(
        const std::string& channel,
        std::int32_t pid
    ) noexcept -> void {
        auto chan = connection->channel(channel);
        if (chan) chan->unignore(pid);
    }

    auto client::unlisten() -> ext::task<> {
        co_await unlisten("*");
    }

    auto client::unlisten(const std::string& channel) -> ext::task<> {
        co_await exec(fmt::format("UNLISTEN {}", channel));

        if (channel == "*") {
            connection->unlisten();
            co_return;
        }

        auto chan = connection->channel(channel);
        if (chan) chan->close();
    }
}
