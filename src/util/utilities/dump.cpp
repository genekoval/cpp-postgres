#include "../commands.hpp"

#include <pg++/pg++>

namespace {
    namespace internal {
        auto async(std::string_view query) -> ext::task<> {
            auto client = co_await pg::connect();

            const auto result = co_await client.fetch<pg::debug::bytes>(
                fmt::format("SELECT {}", query)
            );

            fmt::print("{}\n", result);
        }

        auto dump(const commline::app& app, std::string_view query) -> void {
            netcore::run(async(query));
        }
    }
}

namespace pg::cmd {
    using namespace commline;

    auto dump() -> std::unique_ptr<command_node> {
        return command(
            __FUNCTION__,
            "dump binary field data",
            options(),
            arguments(required<std::string_view>("query")),
            internal::dump
        );
    }
}
