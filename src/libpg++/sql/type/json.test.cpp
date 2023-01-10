#include "type.test.hpp"

using JsonTest = pg::test::TypeTest;

using namespace std::literals;

namespace {
    const std::string_view json = R"({"foo": "bar", "i": 20, "baz": null})";

    auto expect_equal(const pg::json& j) -> void {
        EXPECT_EQ("bar"sv, j["foo"].get<std::string>());
        EXPECT_EQ(20, j["i"].get<std::int32_t>());
        EXPECT_EQ(nullptr, j["baz"]);
    }
}

TEST_F(JsonTest, Read) {
    run([&]() -> ext::task<> {
        const auto result = co_await client.fetch<pg::json>(fmt::format(
            "SELECT '{}'::jsonb",
            json
        ));

        expect_equal(result);
    }());
}

TEST_F(JsonTest, Write) {
    run([&]() -> ext::task<> {
        const auto result = co_await client.query("SELECT $1::jsonb", json);
        const auto j = pg::json::parse(result[0][0].string().value());

        expect_equal(j);
    }());
}
