#include <pg++/connection/type/byte.hpp>
#include <pg++/connection/type/header.hpp>
#include <pg++/connection/type/int.hpp>

namespace pg::detail {
    auto decoder<header>::decode(reader& reader) -> ext::task<header> {
        auto result = header {
            .code = co_await decoder<decltype(header::code)>::decode(reader),
            .len = co_await decoder<decltype(header::len)>::decode(reader)
        };

        TIMBER_TRACE(
            "read header: Byte1('{}') Int32({})",
            result.code,
            result.len
        );

        result.len -= sizeof(decltype(header::len));
        co_return result;
    }
}
