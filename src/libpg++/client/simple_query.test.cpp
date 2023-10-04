#include "client.test.hpp"

using namespace std::literals;

using SimpleQuery = pg::test::ClientTest;

TEST_F(SimpleQuery, OneStatement) {
    run([&]() -> ext::task<> {
        const auto results =
            co_await client->simple_query("SELECT 2 + 2 AS sum");

        EXPECT_EQ(1, results.size());

        const auto& result = results.front();

        EXPECT_EQ(1, result.size());
        EXPECT_EQ("SELECT 1"sv, result.command_tag());

        const auto& row = result[0];

        EXPECT_EQ(1, row.size());

        const auto& field = row[0];

        EXPECT_FALSE(field.is_null());
        EXPECT_EQ("sum"sv, field.name());
        EXPECT_EQ("4"sv, field.string());
    }());
}

TEST_F(SimpleQuery, MultipleStatements) {
    run([&]() -> ext::task<> {
        const auto results = co_await client->simple_query(
            "SELECT 2 + 2 AS sum;"
            "SELECT 42 AS num, NULL::text AS word"
        );

        EXPECT_EQ(2, results.size());

        const auto& first = results[0];

        EXPECT_EQ(1, first.size());
        EXPECT_EQ("SELECT 1"sv, first.command_tag());

        const auto& row = first[0];

        EXPECT_EQ(1, row.size());

        const auto& field = row[0];

        EXPECT_FALSE(field.is_null());
        EXPECT_EQ("sum"sv, field.name());
        EXPECT_EQ("4"sv, field.string());

        const auto& second = results[1];

        EXPECT_EQ(1, second.size());
        EXPECT_EQ("SELECT 1"sv, second.command_tag());

        EXPECT_EQ(2, second[0].size());

        const auto& num = second[0]["num"];

        EXPECT_FALSE(num.is_null());
        EXPECT_EQ("42"sv, num.string());

        const auto& word = second[0]["word"];

        EXPECT_TRUE(word.is_null());
        EXPECT_FALSE(word.string().has_value());
    }());
}

TEST_F(SimpleQuery, EmptyStatement) {
    run([&]() -> ext::task<> {
        const auto results = co_await client->simple_query("");

        EXPECT_EQ(1, results.size());

        const auto& result = results.front();

        EXPECT_TRUE(result.empty());
        EXPECT_EQ(""sv, result.command_tag());
    }());
}
