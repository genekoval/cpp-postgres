#include <pg++/connection/type/byte.hpp>
#include <pg++/connection/type/string.hpp>

namespace pg::detail {
    auto decoder<std::string>::decode(
        reader& reader
    ) -> ext::task<std::string> {
        auto string = std::string();
        auto found_null = false;

        do {
            const auto data = co_await reader.data();
            const auto* chars = reinterpret_cast<const char*>(data.data());

            auto i = 0;

            while (i < data.size() && !found_null) {
                if (chars[i++] == '\0') found_null = true;
            }

            string.append(chars, i - 1);
            reader.advance(i);
        } while (!found_null);

        TIMBER_TRACE(R"(read String("{}"))", string);

        co_return string;
    }

    auto encoder<std::string>::encode(
        std::string_view string,
        writer& writer
    ) -> ext::task<> {
        return encoder<std::string_view>::encode(string, writer);
    }

    auto encoder<std::string>::size(std::string_view string) -> std::int32_t {
        return encoder<std::string_view>::size(string);
    }

    auto encoder<std::string_view>::encode(
        std::string_view string,
        writer& writer
    ) -> ext::task<> {
        constexpr auto null = '\0';

        co_await writer.write(string.data(), string.size());
        co_await writer.write(&null, 1);

        TIMBER_TRACE(R"(write String("{}"))", string);
    }

    auto encoder<std::string_view>::size(
        std::string_view string
    ) -> std::int32_t {
        return string.size() + 1; // Add one for the null terminator.
    }
}
