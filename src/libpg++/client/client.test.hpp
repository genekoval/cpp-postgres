#pragma once

#include <pg++/pg++>

#include <gtest/gtest.h>

namespace pg::test {
    class ClientTest : public testing::Test {
    protected:
        pg::client* client = nullptr;

        auto run(ext::task<>&& task) -> void;
    };
}
