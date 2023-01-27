#include <pg++/connection/channel.hpp>
#include <pg++/connection/connection.hpp>

#include <cassert>

namespace pg::detail {
    channel::channel(
        std::string_view name,
        std::weak_ptr<netcore::mutex<detail::connection>>&& connection
    ) :
        channel_name(name),
        connection(
            std::forward<std::weak_ptr<netcore::mutex<detail::connection>>>(
                connection
            )
        )
    {}

    channel::~channel() {
        if (auto connection = this->connection.lock()) {
            connection->get().unlisten(channel_name);
        }
    }

    auto channel::close() -> void {
        event.cancel();
    }

    auto channel::ignore(std::int32_t pid) -> void {
        ignored.insert(pid);
    }

    auto channel::listen() -> ext::task<std::string> {
        return event.listen();
    }

    auto channel::name() const noexcept -> std::string_view {
        return channel_name;
    }

    auto channel::notify(const std::string& payload, std::int32_t pid) -> void {
        if (ignored.contains(pid)) return;
        event.emit(payload);
    }

    auto channel::unignore(std::int32_t pid) -> void {
        ignored.erase(pid);
    }
}

namespace pg {
    channel::channel(std::shared_ptr<detail::channel>&& inner) :
        inner(std::forward<std::shared_ptr<detail::channel>>(inner))
    {}

    auto channel::ignore(std::int32_t pid) -> void {
        inner->ignore(pid);
    }

    auto channel::listen() -> ext::task<std::string> {
        assert(inner);
        return inner->listen();
    }

    auto channel::name() const noexcept -> std::string_view {
        assert(inner);
        return inner->name();
    }

    auto channel::unignore(std::int32_t pid) -> void {
        inner->unignore(pid);
    }
}
