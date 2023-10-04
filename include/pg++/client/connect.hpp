#pragma once

#include "client.hpp"
#include "parameters.hpp"

namespace pg {
    auto connect() -> ext::task<client>;

    auto connect(const parameters& params) -> ext::task<client>;

    auto connect(const parameters& params, std::size_t buffer_size)
        -> ext::task<client>;
}
