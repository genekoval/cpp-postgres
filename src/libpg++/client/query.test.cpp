#include "client.test.hpp"

using namespace std::literals;

using Query = pg::test::ClientTest;

TEST_F(Query, NoParameters) {
    run([&]() -> ext::task<> {
        const auto result = co_await client->query("SELECT 1 + 1");

        EXPECT_EQ(1, result.size());
        EXPECT_EQ(1, result[0].size());
        EXPECT_EQ("2"sv, result[0][0].string());
        EXPECT_EQ("SELECT 1", result.command_tag());
    }());
}

TEST_F(Query, OneParameter) {
    run([&]() -> ext::task<> {
        const auto result = co_await client->query("SELECT $1 + 2", 4);

        EXPECT_EQ(1, result.size());
        EXPECT_EQ(1, result[0].size());
        EXPECT_EQ("6"sv, result[0][0].string());
        EXPECT_EQ("SELECT 1", result.command_tag());
    }());
}

TEST_F(Query, MultipleParameters) {
    run([&]() -> ext::task<> {
        const auto result = co_await client->query(
            "SELECT $1 + $2 AS sum, $3 AS word",
            5,
            10,
            std::string_view("hello")
        );

        EXPECT_EQ(1, result.size());

        const auto& row = result[0];

        EXPECT_EQ(2, row.size());

        const auto& sum = row["sum"];
        const auto& word = row["word"];

        EXPECT_EQ("15"sv, sum.string());
        EXPECT_EQ("hello"sv, word.string());
    }());
}
