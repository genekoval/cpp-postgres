#include "type.test.hpp"

using namespace std::literals;

using CompositeTest = pg::test::TypeTest;

namespace {
    struct composite {
        std::int64_t id;
        std::string message;
    };
}

PGCPP_COMPOSITE_DECL(::composite, "composite");

PGCPP_COMPOSITE_DEFINE(::composite, &::composite::id, &::composite::message);

TEST_F(CompositeTest, ReadComposite) {
    run([&]() -> ext::task<> {
        const auto result = co_await client->fetch<composite>(
            "SELECT ROW(42::int8, 'hello'::text)"
        );

        EXPECT_EQ(42, result.id);
        EXPECT_EQ("hello"sv, result.message);
    }());
}

TEST_F(CompositeTest, ReadRow) {
    run([&]() -> ext::task<> {
        co_await client->simple_query(
            "CREATE TEMP TABLE composite (id int8, message text);"
            "INSERT INTO composite VALUES (42, 'hello');"
        );

        const auto result =
            co_await client->fetch<composite>("SELECT * FROM composite");

        EXPECT_EQ(42, result.id);
        EXPECT_EQ("hello"sv, result.message);
    }());
}

TEST_F(CompositeTest, WriteComposite) {
    run([&]() -> ext::task<> {
        co_await client->simple_query(
            "CREATE TEMP TABLE composite (id int8, message text)"
        );

        const auto comp = composite {100, "foobar"};
        const auto result = co_await client->query("SELECT $1", comp);

        EXPECT_EQ("(100,foobar)"sv, result[0][0].string());
    }());
}
