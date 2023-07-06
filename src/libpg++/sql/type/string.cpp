#include <pg++/sql/type/string.hpp>

namespace pg {
    auto type<std::string>::from_sql(
        std::int32_t size,
        netcore::buffered_socket& reader
    ) -> ext::task<std::string> {
        auto string = std::string(size, '\0');

        co_await reader.read(string.data(), size);

        TIMBER_TRACE("read SQL string({}): '{}'", size, string);

        co_return string;
    }

    auto type<std::string>::to_sql(
        std::string_view string,
        netcore::buffered_socket& writer
    ) -> ext::task<> {
        return type<std::string_view>::to_sql(string, writer);
    }

    auto type<std::string>::size(std::string_view string) -> std::int32_t {
        return type<std::string_view>::size(string);
    }

    auto type<std::string_view>::to_sql(
        std::string_view string,
        netcore::buffered_socket& writer
    ) -> ext::task<> {
        co_await writer.write(string.data(), string.size());
    }

    auto type<std::string_view>::size(std::string_view string) -> std::int32_t {
        return string.size();
    }

    auto type<const char*>::to_sql(
        const char* string,
        netcore::buffered_socket& writer
    ) -> ext::task<> {
        co_await type<std::string_view>::to_sql(string, writer);
    }

    auto type<const char*>::size(const char* string) -> std::int32_t {
        return type<std::string_view>::size(string);
    }
}
