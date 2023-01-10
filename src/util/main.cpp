#include "commands.hpp"

#include <commline/commline>
#include <timber/timber>

using namespace commline;

namespace {
    namespace internal {
        auto main(const app& app) -> void {
        }
    }
}

auto main(int argc, const char** argv) -> int {
    std::locale::global(std::locale(""));

    timber::log_handler = &timber::console_logger;
    timber::reporting_level = timber::level::info;

    auto app = application(
        "util",
        "0.0.0",
        "pg++ utilities",
        options(),
        arguments(),
        internal::main
    );

    app.subcommand(pg::cmd::dump());
    app.subcommand(pg::cmd::oid());

    return app.run(argc, argv);
}