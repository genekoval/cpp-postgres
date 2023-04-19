#include "client.test.hpp"

using namespace std::literals;

class FetchRows : public pg::test::ClientTest {
protected:
    FetchRows() {
        run([&]() -> ext::task<> {
            co_await client->simple_query(
                "DROP TABLE IF EXISTS fetch_test;"
                "CREATE UNLOGGED TABLE fetch_test (word text);"
                "INSERT INTO fetch_test (word) VALUES "
                    "('hello'), ('world'), ('foo');"
            );
        }());
    }
};

TEST_F(FetchRows, NoParameters) {
    run([&]() -> ext::task<> {
        const auto rows = co_await client->fetch_rows<std::string>(
            "SELECT word FROM fetch_test ORDER BY word"
        );

        EXPECT_EQ(3, rows.size());
        EXPECT_EQ("foo"sv, rows[0]);
        EXPECT_EQ("hello"sv, rows[1]);
        EXPECT_EQ("world"sv, rows[2]);
    }());
}

TEST_F(FetchRows, OneParameter) {
    run([&]() -> ext::task<> {
        const auto rows = co_await client->fetch_rows<std::string>(
            "SELECT word FROM fetch_test WHERE word = $1",
            "foo"
        );

        EXPECT_EQ(1, rows.size());
        EXPECT_EQ("foo"sv, rows[0]);
    }());
}

TEST_F(FetchRows, MultipleParameters) {
    run([&]() -> ext::task<> {
        const auto rows = co_await client->fetch_rows<std::string>(
            "SELECT word || $1 FROM fetch_test WHERE word = $2",
            "bar",
            "foo"
        );

        EXPECT_EQ(1, rows.size());
        EXPECT_EQ("foobar"sv, rows[0]);
    }());
}

TEST_F(FetchRows, ZeroRows) {
    run([&]() -> ext::task<> {
        const auto rows = co_await client->fetch_rows<std::string>(
            "SELECT word FROM fetch_test WHERE word = 'zero'"
        );

        EXPECT_TRUE(rows.empty());
    }());
}
