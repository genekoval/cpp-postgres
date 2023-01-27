#include "client.test.hpp"

class Transaction : public pg::test::ClientTest {
protected:
    static constexpr std::int32_t value = 30;

    auto expect_empty() -> ext::task<> {
        const auto values = co_await rows();

        EXPECT_TRUE(values.empty());
    }

    auto expect_value() -> ext::task<> {
        const auto values = co_await rows();

        EXPECT_EQ(1, values.size());
        EXPECT_EQ(value, values.at(0));
    }

    auto insert() -> ext::task<> {
        co_await client.exec(
            "INSERT INTO transaction_test VALUES ($1)",
            value
        );
    }

    auto rows() -> ext::task<std::vector<std::int32_t>> {
        co_return co_await client.fetch_rows<std::int32_t>(
            "SELECT num FROM transaction_test"
        );
    }

    auto run(ext::task<>&& task) -> void {
        pg::test::ClientTest::run([&](ext::task<>&& task) -> ext::task<> {
            co_await client.simple_query(
                "CREATE TEMP TABLE transaction_test (num int4);"
            );

            co_await std::forward<ext::task<>>(task);
        }(std::forward<ext::task<>>(task)));
    }
};

TEST_F(Transaction, Commit) {
    run([&]() -> ext::task<> {
        auto tx = co_await client.begin();
        co_await insert();
        co_await tx.commit();

        co_await expect_value();
    }());
}

TEST_F(Transaction, ExplicitRollback) {
    run([&]() -> ext::task<> {
        auto tx = co_await client.begin();
        co_await insert();
        co_await tx.rollback();

        co_await expect_empty();
    }());
}

TEST_F(Transaction, DestructorRollback) {
    run([&]() -> ext::task<> {
        {
            auto tx = co_await client.begin();
            co_await insert();
        }

        co_await expect_empty();
    }());
}
