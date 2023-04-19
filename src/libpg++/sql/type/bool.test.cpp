#include "type.test.hpp"

using BoolTest = pg::test::TypeTest;

TEST_F(BoolTest, Read) {
    run([&]() -> ext::task<> {
        auto result = co_await client->fetch<bool>("SELECT true");
        EXPECT_TRUE(result);

        result = co_await client->fetch<bool>("SELECT false");
        EXPECT_FALSE(result);
    }());
}

TEST_F(BoolTest, Write) {
    run([&]() -> ext::task<> {
        auto result = co_await client->query("SELECT $1", true);
        EXPECT_EQ("t", result[0][0].string());

        result = co_await client->query("SELECT $1", false);
        EXPECT_EQ("f", result[0][0].string());
    }());
}
