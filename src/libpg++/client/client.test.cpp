#include "client.test.hpp"

namespace {
    const auto params = pg::parameters {
        .host = "/run/postgresql",
        .params = {
            {"user", "pgtest"},
            {"database", "pgtest"}
        }
    };
}

namespace pg::test {
    auto ClientTest::connect(pg::client& client) -> ext::task<> {
        client = co_await pg::connect(params);
    }

    auto ClientTest::run(ext::task<>&& task) -> void {
        netcore::run([this](ext::task<>&& task) -> ext::task<> {
            co_await connect(client);
            co_await std::forward<ext::task<>>(task);
        }(std::forward<ext::task<>>(task)));
    }
}
