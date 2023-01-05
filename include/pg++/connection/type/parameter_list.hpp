#pragma once

#include "../reader.hpp"
#include "../writer.hpp"

namespace pg {
    using parameter_list = std::unordered_map<std::string, std::string>;
}

namespace pg::detail {
    template <>
    struct encoder<parameter_list> {
        static auto encode(
            const parameter_list& parameters,
            writer& writer
        ) -> ext::task<>;

        static auto size(const parameter_list& parameters) -> std::int32_t;
    };

    static_assert(encodable<parameter_list>);
}
