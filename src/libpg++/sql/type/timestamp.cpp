#include <pg++/sql/type/int.hpp>
#include <pg++/sql/type/timestamp.hpp>

using namespace std::literals;

using int64 = pg::type<std::int64_t>;
using std::chrono::duration_cast;
using std::chrono::microseconds;
using std::chrono::sys_days;
using std::chrono::January;

namespace {
    constexpr auto pg_epoch = January / 1 / 2000;
    constexpr auto time_since_epoch = sys_days{pg_epoch}.time_since_epoch();
    constexpr auto epoch_offset = duration_cast<microseconds>(time_since_epoch);
}

namespace pg {
    auto type<timestamp>::from_sql(
        std::int32_t size,
        reader& reader
    ) -> ext::task<timestamp> {
        const auto value = co_await int64::from_sql(size, reader);
        const auto micros = microseconds(value) + epoch_offset;

        co_return timestamp{micros};
    }

    auto type<timestamp>::to_sql(
        const timestamp time,
        writer& writer
    ) -> ext::task<> {
        const auto time_since_epoch = time.time_since_epoch();
        const auto micros = duration_cast<microseconds>(time_since_epoch);
        const auto converted = micros - epoch_offset;
        const auto value = converted.count();

        co_await int64::to_sql(value, writer);
    }
}
