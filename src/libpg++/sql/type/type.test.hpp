#pragma once

#include "../../client/client.test.hpp"

namespace pg::test {
    class TypeTest : public ClientTest {
    protected:
        template <typename T>
        auto test_read_write(const T& expected) -> void {
            run([&]() -> ext::task<> {
                const auto result = co_await client.fetch<T>(
                    "SELECT $1",
                    expected
                );

                EXPECT_EQ(expected, result);
            }());
        }

        template <typename Result, typename Parameter>
        auto test_write(const Parameter& expected) -> void {
            run([&]() -> ext::task<> {
                const auto result = co_await client.fetch<Result>(
                    "SELECT $1",
                    expected
                );

                EXPECT_EQ(expected, result);
            }());
        }
    };
}
