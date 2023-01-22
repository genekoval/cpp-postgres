#include <pg++/client/connect.hpp>

#include <filesystem>

namespace {
    namespace internal {
        auto connect(
            std::string_view host,
            std::string_view port
        ) -> ext::task<netcore::socket> {
            if (host.starts_with('/')) {
                co_return co_await netcore::connect(host);
            }

            co_return co_await netcore::connect(host, port);
        }
    }
}

namespace pg {
    auto connect() -> ext::task<client> {
        co_return co_await connect(parameters::get());
    }

    auto connect(const parameters& params) -> ext::task<client> {
        auto connection = std::shared_ptr<detail::connection>(
            new detail::connection(
                co_await internal::connect(params.host, params.port)
            )
        );

        co_await connection->startup_message(params.password, params.params);

        detail::run_connection_task(connection);

        co_return pg::client(std::move(connection));
    }
}
