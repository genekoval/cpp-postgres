#include <pg++/sql/type/bool.hpp>

namespace pg {
    auto type<bool>::from_sql(
        std::int32_t size,
        reader& reader
    ) -> ext::task<bool> {
        auto result = false;

        co_await reader.read(&result, sizeof(bool));
        co_return result;
    }

    auto type<bool>::to_sql(bool b, writer& writer) -> ext::task<> {
        co_await writer.write(&b, sizeof(bool));
    }
}
