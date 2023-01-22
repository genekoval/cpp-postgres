#include "client.test.hpp"

namespace pg::test {
    auto ClientTest::run(ext::task<>&& task) -> void {
        netcore::run([this](ext::task<>&& task) -> ext::task<> {
            client = co_await connect();
            co_await std::forward<ext::task<>>(task);
        }(std::forward<ext::task<>>(task)));
    }
}
