#include <pg++/connection/type/byte.hpp>
#include <pg++/connection/type/parameter_list.hpp>
#include <pg++/connection/type/string.hpp>

namespace pg::detail {
    auto encoder<parameter_list>::encode(
        const parameter_list& parameters,
        netcore::buffered_socket& writer
    ) -> ext::task<> {
        for (const auto& [key, value] : parameters) {
            co_await encoder<std::string>::encode(key, writer);
            co_await encoder<std::string>::encode(value, writer);
        }
    }

    auto encoder<parameter_list>::size(
        const parameter_list& parameters
    ) -> std::int32_t {
        std::int32_t sum = 0;

        for (const auto& [key, value] : parameters) {
            sum += encoder<std::string>::size(key);
            sum += encoder<std::string>::size(value);
        }

        return sum;
    }
}
