#include "client.test.hpp"

using Fetch = pg::test::ClientTest;

TEST_F(Fetch, NoParameters) {
    run([&]() -> ext::task<> {
        const auto i = co_await client->fetch<std::int32_t>("SELECT 1 + 1");
        EXPECT_EQ(2, i);
    }());
}

TEST_F(Fetch, OneParameter) {
    run([&]() -> ext::task<> {
        const auto i = co_await client->fetch<std::int32_t>("SELECT $1 + 1", 9);
        EXPECT_EQ(10, i);
    }());
}

TEST_F(Fetch, MultipleParameters) {
    run([&]() -> ext::task<> {
        const auto i =
            co_await client->fetch<std::int32_t>("SELECT $1 + $2", 100, 50);

        EXPECT_EQ(150, i);
    }());
}
