#include "client.test.hpp"

using namespace std::literals;

namespace {
    const auto channel_name = "test_notif"s;
}

class Notification : public pg::test::ClientTest {
protected:
    netcore::event<> listening;
    int notifications = 0;
    std::string payload;

    auto listen() -> ext::task<> {
        listen_task();
        co_await listening.listen();
    }

    auto listen_task() -> ext::detached_task {
        co_await netcore::yield();

        auto channel = co_await client->listen(channel_name);
        listening.emit();

        while (true) {
            try {
                payload = co_await channel.listen();
                ++notifications;
            }
            catch (const netcore::task_canceled&) {
                break;
            }
        }
    }

    auto notify(int notifications, std::string_view payload = "")
        -> ext::task<> {
        const auto query = fmt::format(
            "NOTIFY {}{}",
            channel_name,
            payload.empty() ? "" : fmt::format(", '{}'", payload)
        );

        co_await client->exec(query);
        co_await netcore::yield();

        EXPECT_EQ(notifications, this->notifications);
        EXPECT_EQ(payload, this->payload);
    }
};

TEST_F(Notification, Payload) {
    run([&]() -> ext::task<> {
        constexpr auto payload = "hello"sv;

        co_await listen();
        co_await notify(1, payload);
    }());
}

TEST_F(Notification, Ignore) {
    run([&]() -> ext::task<> {
        const auto pid = client->backend_pid();
        const auto ignored = std::unordered_set<std::int32_t> {pid};

        co_await listen();

        co_await notify(1);

        client->ignore(channel_name, pid);
        co_await notify(1);

        client->unignore(channel_name, pid);
        co_await notify(2);

        client->ignore(ignored);
        co_await notify(2);

        client->unignore();
        co_await notify(3);
    }());
}

TEST_F(Notification, Unlisten) {
    run([&]() -> ext::task<> {
        co_await listen();

        EXPECT_EQ(1, client->listeners(channel_name));

        co_await client->unlisten(channel_name);
        co_await netcore::yield();

        EXPECT_EQ(0, client->listeners(channel_name));
    }());
}
