#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace pg::detail {
    struct passfile_fields {
        std::string_view hostname;
        std::string_view port;
        std::string_view database;
        std::string_view username;
    };

    auto passfile(
        const passfile_fields& fields,
        std::istream& stream
    ) -> std::optional<std::string>;

    auto passfile(
        const passfile_fields& fields,
        const std::optional<std::filesystem::path>& file
    ) -> std::optional<std::string>;
}
