#include "examples.hpp"

#include <commline/commline>
#include <dotenv/dotenv>
#include <timber/timber>

using namespace commline;

namespace {
    namespace internal {
        using example_fn = auto (*)(pg::client& client) -> ext::task<>;

        struct entry {
            std::string_view name;
            std::string_view description;
            example_fn function;
        };

        constexpr auto examples = std::array {
            entry {
                "simple",
                "Simple text queries with multiple commands per query",
                example::simple_query
            }
        };

        auto print_examples() -> void {
            fmt::print("The following examples are available:\n");

            for (const auto& ex : examples) {
                fmt::print("\t{}  {}\n", ex.name, ex.description);
            }
        }

        auto async_main(example_fn example) -> ext::task<> {
            auto client = co_await pg::connect();
            co_await example(client);
        }

        auto main(
            const app& app,
            std::optional<std::string_view> example
        ) -> void {
            if (!example) {
                print_examples();
                return;
            }

            const auto name = *example;

            const auto result = std::find_if(
                examples.begin(),
                examples.end(),
                [name](const entry& entry) -> bool {
                    return entry.name == name;
                }
            );

            if (result == examples.end()) {
                throw std::runtime_error(fmt::format(
                    "example '{}' does not exist",
                    name
                ));
            }

            netcore::async(async_main(result->function));
        }
    }
}

auto main(int argc, const char** argv) -> int {
    std::locale::global(std::locale(""));
    dotenv::load();

    timber::log_handler = &timber::console_logger;

    auto app = application(
        "examples",
        "0.0.0",
        "pg++ examples",
        options(),
        arguments(
            optional<std::string_view>("example")
        ),
        internal::main
    );

    return app.run(argc, argv);
}
