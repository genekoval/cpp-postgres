#include "passfile.hpp"

#include <pg++/client/parameters.hpp>
#include <pg++/except/except.hpp>

#include <filesystem>
#include <pg_config_manual.h>

namespace fs = std::filesystem;

namespace {
    constexpr auto default_port = "5432";
    constexpr auto socket_file = ".s.PGSQL.5432";

    class parser {
        using param = std::pair<std::string, std::string>;

        std::string_view str;
        const char* it;
        const char* const end;

        parser(std::string_view str) :
            str(str),
            it(str.begin()),
            end(str.end()) {}

        auto consume(char expected) -> void {
            if (it == end) throw std::runtime_error("unexpected EOF");

            const auto c = *it;
            if (c == expected) {
                ++it;
                return;
            }

            throw std::runtime_error(fmt::format(
                "unexpected character at byte {}: expected `{}` but got `{}`",
                it - str.begin(),
                expected,
                c
            ));
        }

        auto keyword() -> std::optional<std::string> {
            const auto* start = it;

            while (it != end) {
                const unsigned char c = *it;
                if (c == '=' || std::isspace(c)) break;

                ++it;
            }

            if (start == it) return std::nullopt;
            return std::string(start, it);
        }

        auto parameter() -> std::optional<param> {
            skip_whitespace();

            auto key = keyword();
            if (!key) return std::nullopt;

            skip_whitespace();
            consume('=');
            skip_whitespace();

            return std::optional<param>(
                std::in_place,
                std::move(*key),
                value()
            );
        }

        auto quoted_value() -> std::string {
            consume('\'');

            auto val = std::string();

            while (it != end) {
                const auto c = *it;

                if (c == '\'') {
                    consume('\'');
                    return val;
                }

                ++it;

                if (c == '\\') {
                    if (it != end) val.push_back(*it++);
                }
                else val.push_back(c);
            }

            throw std::runtime_error(
                "unterminated quoted connection parameter value"
            );
        }

        auto simple_value() -> std::string {
            auto val = std::string();

            while (it != end) {
                const unsigned char c = *it;
                if (std::isspace(c)) break;
                ++it;

                if (c == '\\') {
                    if (it != end) val.push_back(*it++);
                }
                else val.push_back(c);
            }

            return val;
        }

        auto skip_whitespace() -> void {
            while (it != end) {
                if (std::isspace(*it)) ++it;
                else return;
            }
        }

        auto value() -> std::string {
            if (it == end) throw std::runtime_error("unexpected EOF");

            if (*it == '\'') return quoted_value();
            else return simple_value();
        }
    public:
        static auto parse(std::string_view str) -> pg::parameter_list {
            auto parser = ::parser {str};
            auto params = pg::parameter_list();

            while (auto param = parser.parameter()) {
                params.insert(std::move(*param));
            }

            return params;
        }
    };

    constexpr auto default_host() -> const char* {
        const auto dir = std::string_view(DEFAULT_PGSOCKET_DIR);
        if (dir.empty()) return "localhost";
        return dir.data();
    }

    auto env() -> pg::parameter_list {
        using env_pair = std::pair<std::string_view, std::string_view>;

        auto result = pg::parameter_list();

        const auto getenv =
            [&result](std::string_view var, std::string_view key) {
                if (const auto* value = std::getenv(var.data())) {
                    result[std::string(key)] = value;
                }
            };

        for (const auto& [var, key] : std::initializer_list<env_pair> {
                 {"PGHOST", "host"},
                 {"PGHOSTADDR", "hostaddr"},
                 {"PGPORT", "port"},
                 {"PGDATABASE", "dbname"},
                 {"PGUSER", "user"},
                 {"PGPASSWORD", "password"},
                 {"PGPASSFILE", "passfile"},
                 {"PGCHANNELBINDING", "channel_binding"},
                 {"PGOPTIONS", "options"},
                 {"PGAPPNAME", "application_name"},
                 {"PGSSLMODE", "sslmode"},
                 {"PGSSLCOMPRESSION", "sslcompression"},
                 {"PGSSLCERT", "sslcert"},
                 {"PGSSLKEY", "sslkey"},
                 {"PGSSLROOTCERT", "sslrootcert"},
                 {"PGSSLCRL", "sslcrl"},
                 {"PGSSLCRLDIR", "sslcrldir"},
                 {"PGSSLSNI", "sslsni"},
                 {"PGREQUIREPEER", "requirepeer"},
                 {"PGSSLMINPROTOCOLVERSION", "ssl_min_protocol_version"},
                 {"PGSSLMAXPROTOCOLVERSION", "ssl_max_protocol_version"},
                 {"PGGSSENCMODE", "gssencmode"},
                 {"PGKRBSRVNAME", "krbsrvname"},
                 {"PGGSSLIB", "gsslib"},
                 {"PGCONNECT_TIMEOUT", "connect_timeout"},
                 {"PGCLIENTENCODING", "client_encoding"},
                 {"PGTARGETSESSIONATTRS", "target_session_attrs"}})
            getenv(var, key);

        return result;
    }

    auto parse(pg::parameter_list& params) -> pg::parameters {
        params.merge(env());

        const auto find = [&params](const std::string& key
                          ) -> std::optional<std::string> {
            const auto result = params.find(key);
            if (result == params.end()) return std::nullopt;
            return result->second;
        };

        auto host = find("host").value_or(default_host());
        if (host.starts_with('/')) {
            auto path = fs::path(host);

            if (fs::status(path).type() == fs::file_type::directory) {
                path /= socket_file;
                host = path.string();
            }
        }

        auto result = pg::parameters {
            .host = host,
            .port = find("port").value_or(default_port)};

        auto user = std::string();
        if (const auto value = find("user")) user = std::move(*value);
        else if (const auto* login = getlogin()) user = login;
        else throw pg::error("no value provided for 'user'");
        result.params["user"] = user;

        const auto database = find("dbname").value_or(user);
        result.params["database"] = database;

        if (const auto password = find("password")) {
            result.password = *password;
        }
        else if (const auto password = pg::detail::passfile({
                .hostname =  result.host,
                .port = result.port,
                .database = database,
                .username = user,
            },
            find("passfile")
        ))
            result.password = std::move(*password);

        for (const auto& key : {"application_name", "client_encoding"}) {
            if (const auto value = find(key)) { result.params[key] = *value; }
        }

        return result;
    }
}

namespace pg {
    auto parameters::get() -> parameters {
        auto params = parameter_list();
        return ::parse(params);
    }

    auto parameters::parse(parameter_list&& params) -> parameters {
        auto local = std::forward<parameter_list>(params);
        return ::parse(local);
    }

    auto parameters::parse(std::string_view connection_string) -> parameters {
        return parse(parser::parse(connection_string));
    }
}
