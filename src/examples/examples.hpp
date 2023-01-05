#pragma once

#include <pg++/pg++>

namespace example {
    auto simple_query(pg::client& client) -> ext::task<>;
}
