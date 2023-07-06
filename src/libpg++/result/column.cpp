#include <pg++/connection/type/int.hpp>
#include <pg++/connection/type/string.hpp>
#include <pg++/result/column.hpp>

namespace pg::detail {
    auto decoder<column>::decode(
        netcore::buffered_socket& reader
    ) -> ext::task<column> {
        auto col = column();

        col.name = co_await decoder<std::string>::decode(reader);
        col.table = co_await decoder<std::int32_t>::decode(reader);
        col.column = co_await decoder<std::int16_t>::decode(reader);
        col.type = co_await decoder<std::int32_t>::decode(reader);
        col.size = co_await decoder<std::int16_t>::decode(reader);
        col.modifier = co_await decoder<std::int32_t>::decode(reader);
        col.format = static_cast<format_code>(
            co_await decoder<std::int16_t>::decode(reader)
        );

        co_return col;
    }
}
