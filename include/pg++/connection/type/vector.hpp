#pragma once

#include "int.hpp"

namespace pg::detail {
    template <decodable T>
    struct decoder<std::vector<T>> {
        static auto decode(reader& reader) -> ext::task<std::vector<T>> {
            const auto size = co_await decoder<std::int16_t>::decode(reader);

            auto result = std::vector<T>();
            result.reserve(size);

            for (auto i = 0; i < size; ++i) {
                result.emplace_back(co_await decoder<T>::decode(reader));
            }

            co_return result;
        }
    };
}
