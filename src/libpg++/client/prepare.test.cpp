#include "client.test.hpp"

using Prepare = pg::test::ClientTest;

namespace {
    constexpr auto create_add = std::string_view(R"(
        CREATE OR REPLACE FUNCTION add(integer, integer) RETURNS integer
            AS 'select $1 + $2;'
            LANGUAGE SQL
            IMMUTABLE
            RETURNS NULL ON NULL INPUT)"
    );

    class prepare_test {
        std::reference_wrapper<pg::client> client;
    public:
        prepare_test(pg::client& client) : client(client) {}

        auto add(std::int32_t a, std::int32_t b) -> ext::task<std::int32_t> {
            co_return co_await client.get()
                .fetch_prepared<std::int32_t>("add", a, b);
        }
    };
}

TEST_F(Prepare, Method) {
    run([&]() -> ext::task<> {
        co_await client.exec(create_add);

        co_await client.prepare_fn("add", &prepare_test::add);

        auto instance = prepare_test(client);
        const auto result = co_await instance.add(8, 4);

        EXPECT_EQ(12, result);
    }());
}
