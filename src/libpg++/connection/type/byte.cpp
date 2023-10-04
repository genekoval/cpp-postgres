#include <pg++/connection/type/byte.hpp>

namespace pg::detail {
    auto decoder<char>::decode(netcore::buffered_socket& reader)
        -> ext::task<char> {
        char result = '\0';
        co_await reader.read(&result, 1);

        TIMBER_TRACE(
            "read Byte1('{}')",
            result == '\0' ? R"(\0)" : fmt::format("{}", result)
        );

        co_return result;
    }

    auto encoder<char>::encode(char c, netcore::buffered_socket& writer)
        -> ext::task<> {
        co_await writer.write(&c, 1);

        TIMBER_TRACE(
            "write Byte1('{}')",
            c == '\0' ? R"(\0)" : fmt::format("{}", c)
        );
    }

    auto encoder<char>::size(char c) -> std::int32_t { return 1; }
}
