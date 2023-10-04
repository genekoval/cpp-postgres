#pragma once

#include "int.hpp"

#include <chrono>
#include <fmt/chrono.h>

namespace pg::detail {
    constexpr auto epoch_offset = [] {
        using std::chrono::January;
        using std::chrono::duration_cast;
        using std::chrono::microseconds;
        using std::chrono::sys_days;

        constexpr auto pg_epoch = January / 1 / 2000;

        constexpr auto time_since_epoch = sys_days(pg_epoch).time_since_epoch();

        return duration_cast<microseconds>(time_since_epoch);
    }();
}

namespace pg {
    template <typename Duration>
    struct type<std::chrono::time_point<std::chrono::system_clock, Duration>> {
        using time_point =
            std::chrono::time_point<std::chrono::system_clock, Duration>;

        static constexpr std::int32_t oid = 1184;
        static constexpr std::string_view name = "timestamptz";

        static auto from_sql(
            std::int32_t size,
            netcore::buffered_socket& reader
        ) -> ext::task<time_point> {
            using int64 = type<std::int64_t>;
            using detail::epoch_offset;
            using std::chrono::duration_cast;
            using std::chrono::microseconds;

            const auto value = co_await int64::from_sql(size, reader);
            const auto micros = microseconds(value) + epoch_offset;
            const auto duration = duration_cast<Duration>(micros);

            auto result = time_point(duration);

            TIMBER_TRACE("read SQL timestamptz: {}", result);

            co_return result;
        }

        static auto to_sql(
            const time_point time,
            netcore::buffered_socket& writer
        ) -> ext::task<> {
            using int64 = type<std::int64_t>;
            using detail::epoch_offset;
            using std::chrono::duration_cast;
            using std::chrono::microseconds;

            const auto time_since_epoch = time.time_since_epoch();
            const auto micros = duration_cast<microseconds>(time_since_epoch);
            const auto converted = micros - epoch_offset;
            const auto value = converted.count();

            co_await int64::to_sql(value, writer);
        }

        static constexpr auto size(const time_point time) -> std::int32_t {
            return sizeof(std::int64_t);
        }
    };
}
