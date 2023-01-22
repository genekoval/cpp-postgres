#pragma once

#include "client.hpp"
#include "parameters.hpp"

namespace pg {
    auto connect() -> ext::task<client>;

    auto connect(const parameters& params) -> ext::task<client>;
}
