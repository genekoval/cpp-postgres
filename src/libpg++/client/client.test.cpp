#include "client.test.hpp"

namespace pg::test {
    auto ClientTest::run(ext::task<>&& task) -> void {
        netcore::run([this](ext::task<>&& task) -> ext::task<> {
            auto client = co_await connect();
            this->client = &client;
            co_await std::forward<ext::task<>>(task);
        }(std::forward<ext::task<>>(task)));
    }
}
