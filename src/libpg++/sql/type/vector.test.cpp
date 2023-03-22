#include "type.test.hpp"

using VectorTest = pg::test::TypeTest;

TEST_F(VectorTest, ReadValues) {
    run([&]() -> ext::task<> {
        const auto result = co_await client.fetch<std::vector<std::int16_t>>(
            "SELECT '{1,2,3}'::int2[]"
        );

        EXPECT_EQ(3, result.size());
        EXPECT_EQ(1, result.at(0));
        EXPECT_EQ(2, result.at(1));
        EXPECT_EQ(3, result.at(2));
    }());
}

TEST_F(VectorTest, WriteValues) {
    run([&]() -> ext::task<> {
        const auto value = std::vector<std::int16_t> { 1, 2, 3 };

        const auto result = co_await client.query("SELECT $1", value);

        EXPECT_EQ("{1,2,3}", result[0][0].string());
    }());
}

TEST_F(VectorTest, ReadEmpty) {
    run([&]() -> ext::task<> {
        const auto result = co_await client.fetch<std::vector<std::int16_t>>(
            "SELECT '{}'::int2[]"
        );

        EXPECT_TRUE(result.empty());
    }());
}

TEST_F(VectorTest, ReadNull) {
    run([&]() -> ext::task<> {
        const auto result = co_await client.fetch<std::vector<std::int64_t>>(
            "SELECT NULL::int2[]"
        );

        EXPECT_TRUE(result.empty());
    }());
}

TEST_F(VectorTest, WriteEmpty) {
    run([&]() -> ext::task<> {
        const auto value = std::vector<std::int16_t>();

        const auto result = co_await client.query("SELECT $1", value);

        EXPECT_EQ("{}", result[0][0].string());
    }());
}
