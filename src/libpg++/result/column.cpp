#include <pg++/connection/type/int.hpp>
#include <pg++/connection/type/string.hpp>
#include <pg++/result/column.hpp>

namespace pg::detail {
    auto decoder<column>::decode(reader& reader) -> ext::task<column> {
        co_return column {
            .name = co_await decoder<std::string>::decode(reader),
            .table = co_await decoder<std::int32_t>::decode(reader),
            .column = co_await decoder<std::int16_t>::decode(reader),
            .type = co_await decoder<std::int32_t>::decode(reader),
            .size = co_await decoder<std::int16_t>::decode(reader),
            .modifier = co_await decoder<std::int32_t>::decode(reader),
            .format = static_cast<format_code>(
                co_await decoder<std::int16_t>::decode(reader)
            )
        };
    }
}
