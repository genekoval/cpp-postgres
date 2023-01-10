#pragma once

#include <pg++/pg++>

namespace pg::util {
    auto connect() -> ext::task<pg::client>;
}
