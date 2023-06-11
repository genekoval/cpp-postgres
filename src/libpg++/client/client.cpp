#include <pg++/client/client.hpp>

namespace pg {
    client::client(
        std::shared_ptr<netcore::mutex<detail::connection>> connection
    ) :
        handle(
            std::forward<std::shared_ptr<netcore::mutex<detail::connection>>>(
                connection
            )
        )
    {}

    auto client::backend_pid() const noexcept -> std::int32_t {
        return handle.get().backend_pid();
    }

    auto client::begin() -> ext::task<transaction> {
        auto connection = co_await handle.lock();
        co_await connection->exec("BEGIN");

        TIMBER_DEBUG("{} begin transaction", *connection);

        co_return transaction(handle.shared());
    }

    auto client::ignore(
        const std::unordered_set<std::int32_t>& ignored
    ) noexcept -> void {
        handle.get().ignore(&ignored);
    }

    auto client::ignore(
        const std::string& channel,
        std::int32_t pid
    ) noexcept -> void {
        auto chan = handle.get().find_channel(channel);
        if (chan) chan->ignore(pid);
    }

    auto client::listen(const std::string& channel) -> ext::task<pg::channel> {
        auto connection = co_await handle.lock();
        auto chan = connection->find_channel(channel);
        if (chan) co_return chan;

        co_await connection->exec(fmt::format("LISTEN {}", channel));

        chan = std::shared_ptr<detail::channel>(
            new detail::channel(channel, handle.weak())
        );

        connection->listen(channel, chan);
        co_return chan;
    }

    auto client::listeners(const std::string& channel) -> long {
        return handle.get().listeners(channel);
    }

    auto client::on_notice(notice_callback_type&& callback) -> void {
        handle.get().on_notice(std::forward<notice_callback_type>(callback));
    }

    auto client::simple_query(
        std::string_view query
    ) -> ext::task<std::vector<result>> {
        auto connection = co_await handle.lock();
        co_return co_await connection->simple_query(query);
    }

    auto client::unignore() noexcept -> void {
        handle.get().ignore(nullptr);
    }

    auto client::unignore(
        const std::string& channel,
        std::int32_t pid
    ) noexcept -> void {
        auto chan = handle.get().find_channel(channel);
        if (chan) chan->unignore(pid);
    }

    auto client::unlisten() -> ext::task<> {
        co_await unlisten("*");
    }

    auto client::unlisten(const std::string& channel) -> ext::task<> {
        auto connection = co_await handle.lock();
        co_await connection->exec(fmt::format("UNLISTEN {}", channel));

        if (channel == "*") {
            connection->unlisten();
            co_return;
        }

        auto chan = connection->find_channel(channel);
        if (chan) chan->close();
    }
}
