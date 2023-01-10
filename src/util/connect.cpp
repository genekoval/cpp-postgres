#include "connect.hpp"

namespace pg::util {
    auto connect() -> ext::task<pg::client> {
        const auto params = pg::parameters {
            .host = "/run/postgresql",
            .params = {
                {"user", "pgtest"},
                {"database", "pgtest"}
            }
        };

        co_return co_await pg::connect(params);
    }
}
