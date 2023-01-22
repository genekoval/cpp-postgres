#include "passfile.hpp"

#include <pg++/client/parameters.hpp>
#include <pg++/except/except.hpp>

#include <filesystem>
#include <pg_config_manual.h>

namespace fs = std::filesystem;

namespace {
    constexpr auto default_port = "5432";
    constexpr auto socket_file = ".s.PGSQL.5432";

    constexpr auto default_host() -> const char* {
        const auto dir = std::string_view(DEFAULT_PGSOCKET_DIR);
        if (dir.empty()) return "localhost";
        return dir.data();
    }

    auto env() -> pg::parameter_list {
        using env_pair = std::pair<std::string_view, std::string_view>;

        auto result = pg::parameter_list();

        const auto getenv = [&result](
            std::string_view var,
            std::string_view key
        ) {
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
            {"PGTARGETSESSIONATTRS", "target_session_attrs"}
        }) getenv(var, key);

        return result;
    }

    auto parse(pg::parameter_list& params) -> pg::parameters {
        params.merge(env());

        const auto find = [&params](
            const std::string& key
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
            .port = find("port").value_or(default_port)
        };

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
        )) result.password = std::move(*password);

        for (const auto& key : {
            "application_name",
            "client_encoding"
        }) {
            if (const auto value = find(key)) {
                result.params[key] = *value;
            }
        }

        return result;
    }
}

namespace pg {
    auto parameters::get() -> parameters {
        auto params = parameter_list();
        return ::parse(params);
    }

    auto parameters::parse(const parameter_list& params) -> parameters {
        auto copy = params;
        return ::parse(copy);
    }
}
