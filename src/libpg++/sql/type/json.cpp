#include <pg++/connection/type/byte.hpp>
#include <pg++/sql/type/json.hpp>
#include <pg++/sql/type/string.hpp>

namespace pg {
    auto type<json>::from_sql(
        std::int32_t size,
        reader& reader
    ) -> ext::task<json> {
        const auto version = static_cast<std::int8_t>(
            co_await detail::decoder<char>::decode(reader)
        );

        if (version != type<json>::version) {
            throw bad_conversion(fmt::format(
                "unsupported jsonb version number {}",
                version
            ));
        }

        const auto string = co_await type<std::string>::from_sql(
            size - 1,
            reader
        );

        co_return json::parse(string);
    }

    auto type<json>::to_sql(const json& j, writer& writer) -> ext::task<> {
        co_await detail::encoder<char>::encode(version, writer);

        const auto string = j.dump();
        co_await type<std::string_view>::to_sql(string, writer);
    }

    auto type<json>::size(const json& j) -> std::int32_t {
        return j.dump().size() + 1; // add one byte for version number
    }
}
