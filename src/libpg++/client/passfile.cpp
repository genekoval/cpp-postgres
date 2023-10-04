#include "passfile.hpp"

#include <pg++/except/except.hpp>

#include <ext/string.h>
#include <fmt/core.h>
#include <fstream>
#include <timber/timber>

namespace fs = std::filesystem;

using enum fs::perms;

namespace {
    constexpr std::string_view delimiter = ":";
    constexpr std::string_view wildcard = "*";
    constexpr auto insecure_permissions = group_all | others_all;

    auto default_passfile() -> std::optional<fs::path> {
        if (const auto* home = std::getenv("HOME")) {
            return fs::path(home) / ".pgpass";
        }

        return std::nullopt;
    }

    auto is_valid_passfile(const fs::path& file) -> bool {
        const auto status = fs::status(file);

        if (status.type() != fs::file_type::regular) return false;

        if ((status.permissions() & insecure_permissions) != none) {
            TIMBER_WARNING(
                R"(password file "{}" has group or world access; )"
                "permissions should be u=rw (0600) or less",
                file.native()
            );

            return false;
        }

        return true;
    }

    auto next_token(
        ext::string_range::iterator& it,
        ext::string_range::iterator end
    ) -> std::string {
        auto result = std::ostringstream();

        while (it != end) {
            const auto token = *it++;

            if (token.ends_with('\\')) {
                result << token.substr(0, token.size() - 1) << delimiter;
            }
            else {
                result << token;
                break;
            }
        }

        return result.str();
    }
}

namespace pg::detail {
    auto passfile(const passfile_fields& fields, std::istream& stream)
        -> std::optional<std::string> {
        for (std::string line; std::getline(stream, line);) {
            const auto trimmed = ext::trim(line);

            if (trimmed.empty() || trimmed.starts_with('#')) continue;

            auto range = ext::string_range(trimmed, delimiter);
            auto it = range.begin();
            const auto end = range.end();

            auto skip = false;

            for (const auto field :
                 {fields.hostname,
                  fields.port,
                  fields.database,
                  fields.username}) {
                const auto token = next_token(it, end);

                if (token.empty() || (token != wildcard && token != field)) {
                    skip = true;
                    break;
                }
            }

            if (!skip) return next_token(it, end);
        }

        return std::nullopt;
    }

    auto passfile(
        const passfile_fields& fields,
        const std::optional<fs::path>& file
    ) -> std::optional<std::string> {
        auto path = fs::path();

        if (file) path = std::move(*file);
        else if (const auto file = default_passfile()) path = std::move(*file);
        else return std::nullopt;

        if (!is_valid_passfile(path)) return std::nullopt;

        auto stream = std::ifstream(path);
        return passfile(fields, stream);
    }
}
