#include "type.test.hpp"

using namespace std::literals;

namespace {
    enum class mood {
        sad,
        ok,
        happy
    };
}

template <>
struct pg::enum_type<mood> {
    static auto from_string(std::string_view string) -> mood {
        using enum mood;

        static const auto mapping = std::unordered_map<std::string_view, mood> {
            {"sad", sad},
            {"ok", ok},
            {"happy", happy}
        };

        const auto it = mapping.find(string);

        if (it == mapping.end()) throw bad_conversion(fmt::format(
            R"(received invalid value for enum mood: "{}")",
            string
        ));

        return it->second;
    }

    static auto to_string(mood m) -> std::string_view {
        using enum mood;

        switch (m) {
            case sad: return "sad";
            case ok: return "ok";
            case happy: return "happy";
        }
    }
};

PG_ENUM(mood, "mood");

PG_ENUM_DEFINE(mood);

class EnumTest : public pg::test::TypeTest {
    auto read(std::string_view value) -> mood {
        auto result = mood::sad;

        run([&]() -> ext::task<> {
            result = co_await client->fetch<mood>(fmt::format(
                "SELECT '{}'::mood",
                value
            ));
        }());

        return result;
    }

    auto write(mood m) -> std::string {
        auto result = pg::result();

        run([&]() -> ext::task<> {
            result = co_await client->query("SELECT $1", m);
        }());

        return std::string{result[0][0].string().value()};
    }
protected:
    static auto SetUpTestSuite() -> void {
        netcore::run([]() -> ext::task<> {
            auto client = co_await pg::connect();

            co_await client.simple_query(
                "DROP TYPE IF EXISTS mood;"
                "CREATE TYPE mood AS ENUM ('sad', 'ok', 'happy');"
            );
        }());
    }

    auto test_read(std::string_view value, mood expected) -> void {
        const auto result = read(value);
        EXPECT_EQ(expected, result);
    }

    auto test_write(mood value, std::string_view expected) -> void {
        const auto result = write(value);
        EXPECT_EQ(expected, result);
    }
};

TEST_F(EnumTest, Read) {
    test_read("happy", mood::happy);
    test_read("sad", mood::sad);
    test_read("ok", mood::ok);
}

TEST_F(EnumTest, Write) {
    test_write(mood::happy, "happy");
    test_write(mood::sad, "sad");
    test_write(mood::ok, "ok");
}
