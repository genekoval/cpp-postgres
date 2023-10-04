#include "../commands.hpp"

#include <pg++/pg++>

namespace {
    namespace internal {
        auto async(std::string_view query) -> ext::task<> {
            auto client = co_await pg::connect();

            const auto result = co_await client.query(query);

            for (const auto& row : result) {
                auto vec = std::vector<std::string>();

                for (const auto& field : row) {
                    vec.push_back(fmt::format(
                        "(oid: {}, value: {})",
                        field.type(),
                        field.string().value_or("[null]")
                    ));
                }

                fmt::print("{}\n", fmt::join(vec, " "));
            }
        }

        auto oid(const commline::app& app, std::string_view query) -> void {
            netcore::run(async(query));
        }
    }
}

namespace pg::cmd {
    using namespace commline;

    auto oid() -> std::unique_ptr<command_node> {
        return command(
            __FUNCTION__,
            "print field OID",
            options(),
            arguments(required<std::string_view>("query")),
            internal::oid
        );
    }
}
