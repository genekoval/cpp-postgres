#include "type.test.hpp"

using namespace std::literals;

using TupleTest = pg::test::TypeTest;

TEST_F(TupleTest, ReadComposite) {
    run([&]() -> ext::task<> {
        const auto tuple = co_await client->fetch<std::tuple<
            std::string,
            std::int64_t
        >>("SELECT ROW('hello'::text, 80::int8)");

        EXPECT_EQ("hello"sv, std::get<0>(tuple));
        EXPECT_EQ(std::int64_t(80), std::get<1>(tuple));
    }());
}

TEST_F(TupleTest, ReadRow) {
    run([&]() -> ext::task<> {
        co_await client->simple_query(
            "CREATE TEMP TABLE tuple_test (i int4, j int2, t text);"
            "INSERT INTO tuple_test VALUES (50, 100, 'hello');"
        );

        const auto row = co_await client->fetch<std::tuple<
            std::int32_t,
            std::int16_t,
            std::string
        >>("SELECT * FROM tuple_test");

        EXPECT_EQ(50, std::get<0>(row));
        EXPECT_EQ(100, std::get<1>(row));
        EXPECT_EQ("hello"sv, std::get<2>(row));
    }());
}
