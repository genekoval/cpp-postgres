#include "client.test.hpp"

using namespace std::literals;

class Portal : public pg::test::ClientTest {
protected:
    auto run(ext::task<>&& task) -> void {
        pg::test::ClientTest::run([&](ext::task<>&& task) -> ext::task<> {
            co_await client->simple_query(
                "CREATE TEMP TABLE cursor_test (message text);"
                "INSERT INTO cursor_test VALUES ('foo'), ('bar'), ('baz');"
            );
            co_await std::forward<ext::task<>>(task);
        }(std::forward<ext::task<>>(task)));
    }
};

TEST_F(Portal, StreamRows) {
    run([&]() -> ext::task<> {
        auto portal = co_await client->stream<std::string>(
            "StreamRows",
            "SELECT message FROM cursor_test ORDER BY message",
            2
        );

        auto messages = std::span<const std::string>();

        EXPECT_FALSE(portal.done());
        EXPECT_TRUE(portal);

        messages = co_await portal.next();

        EXPECT_FALSE(portal.done());
        EXPECT_TRUE(portal);

        EXPECT_EQ(2, messages.size());
        if (messages.size() < 2) co_return;

        EXPECT_EQ("bar"sv, messages[0]);
        EXPECT_EQ("baz"sv, messages[1]);

        messages = co_await portal.next();

        EXPECT_TRUE(portal.done());
        EXPECT_FALSE(portal);

        EXPECT_EQ(1, messages.size());
        if (messages.size() < 1) co_return;

        EXPECT_EQ("foo"sv, messages[0]);

        messages = co_await portal.next();

        EXPECT_TRUE(portal.done());
        EXPECT_FALSE(portal);

        EXPECT_TRUE(messages.empty());
    }());
}
