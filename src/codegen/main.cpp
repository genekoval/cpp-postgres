#include <commline/commline>

using namespace commline;

namespace {
    namespace internal {
        auto main(const app& app) -> void {}
    }
}

namespace gen {
    auto errcodes() -> std::unique_ptr<command_node>;
}

auto main(int argc, char** argv) -> int {
    std::locale::global(std::locale(""));

    auto app = application(
        NAME,
        VERSION,
        DESCRIPTION,
        options(),
        arguments(),
        internal::main
    );

    app.subcommand(gen::errcodes());

    return app.run(argc, argv);
}
