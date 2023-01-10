#include "type.test.hpp"

using UuidTest = pg::test::TypeTest;

namespace {
    const pg::uuid uuid = "8ea0915f-ac5b-4598-802d-ff74b42c0e41";
}

TEST_F(UuidTest, Read) {
    run([&]() -> ext::task<> {
        const auto result = co_await client.fetch<pg::uuid>(fmt::format(
            "SELECT '{}'::uuid",
            uuid
        ));

        EXPECT_EQ(uuid, result);
    }());
}

TEST_F(UuidTest, Write) {
    run([&]() -> ext::task<> {
        const auto result = co_await client.query("SELECT $1", uuid);

        EXPECT_EQ(uuid.string(), result[0][0].string());
    }());
}
