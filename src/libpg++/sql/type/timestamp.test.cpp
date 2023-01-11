#include "type.test.hpp"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace {
    constexpr auto format_string = "%Y-%m-%d %H:%M:%S";
    constexpr auto time_zone = -5;
}

class TimestampTest : public pg::test::TypeTest {
    auto format(pg::timestamp ts) -> std::string {
        const auto time_t = pg::timestamp::clock::to_time_t(ts);
        const auto* tm = localtime(&time_t);

        auto stream = std::ostringstream();
        stream << std::put_time(tm, format_string);

        return stream.str();
    }

    auto read(std::string_view timestamp) -> pg::timestamp {
        auto result = pg::timestamp();

        run([&]() -> ext::task<> {
            result = co_await client.fetch<pg::timestamp>(fmt::format(
                "SELECT '{}'::timestamptz",
                timestamp
            ));
        }());

        return result;
    }

    auto write(pg::timestamp ts) -> std::string {
        auto result = pg::result();

        run([&]() -> ext::task<> {
            co_await client.query(fmt::format("SET TIME ZONE {}", time_zone));

            result = co_await client.query(
                "SELECT $1::timestamptz",
                ts
            );
        }());

        return std::string{result[0][0].string().value()};
    }
protected:
    auto read_time(std::string_view timestamp) -> void {
        const auto ts = read(timestamp);
        const auto string = format(ts);

        EXPECT_EQ(timestamp, string);
    }

    auto write_time(pg::timestamp ts, std::string_view expected) -> void {
        const auto result = write(ts);

        EXPECT_EQ(expected, result);
    }
};

TEST_F(TimestampTest, Read) {
    read_time("2010-05-25 17:30:26");
}

TEST_F(TimestampTest, Write) {
    using std::chrono::March;
    using std::chrono::sys_days;

    const auto date = March / 8 / 2022;
    const pg::timestamp ts = sys_days{date};

    write_time(ts, "2022-03-07 19:00:00-05");
}
