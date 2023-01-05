#include <pg++/client/connect.hpp>

#include <filesystem>

namespace fs = std::filesystem;

namespace {
    namespace internal {
        constexpr auto default_port = "5432";
        constexpr auto socket_file = ".s.PGSQL.5432";

        auto connect(
            std::string_view host,
            std::optional<std::string_view> port
        ) -> ext::task<netcore::socket> {
            if (host.starts_with('/')) {
                auto path = fs::path(host);

                if (fs::status(path).type() == fs::file_type::directory) {
                    path /= socket_file;
                }

                co_return co_await netcore::connect(path.native());
            }

            co_return co_await netcore::connect(
                host,
                port.value_or(default_port)
            );
        }
    }
}

namespace pg {
    auto connect(const parameters& params) -> ext::task<client> {
        auto connection = std::shared_ptr<detail::connection>(
            new detail::connection(
                co_await internal::connect(params.host, params.port)
            )
        );

        co_await connection->startup_message(params.params);

        detail::run_connection_task(connection);

        co_return pg::client(std::move(connection));
    }
}
