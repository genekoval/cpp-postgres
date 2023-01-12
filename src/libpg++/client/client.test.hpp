#pragma once

#include <pg++/pg++>

#include <gtest/gtest.h>

namespace pg::test {
    class ClientTest : public testing::Test {
    protected:
        static auto connect(pg::client& client) -> ext::task<>;

        pg::client client;

        auto run(ext::task<>&& task) -> void;
    };
}
