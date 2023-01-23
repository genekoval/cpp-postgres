#include <dotenv/dotenv>
#include <filesystem>
#include <fmt/chrono.h>
#include <fmt/os.h>
#include <gtest/gtest.h>
#include <netcore/netcore>
#include <timber/timber>

namespace fs = std::filesystem;

using testing::InitGoogleTest;

namespace {
    const auto log_path = fs::temp_directory_path() / "pg++.test.log";
    auto log_file = fmt::output_file(log_path.native());

    auto file_logger(const timber::log& log) noexcept -> void {
        log_file.print("{:%b %d %r}", log.timestamp);
        log_file.print(" {:>9}  ", log.log_level);
        log_file.print("{}\n", log.message);

        log_file.flush();
    }
}

auto main(int argc, char** argv) -> int {
    dotenv::load();

    timber::log_handler = &file_logger;

    const auto runtime = netcore::runtime();

    InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
