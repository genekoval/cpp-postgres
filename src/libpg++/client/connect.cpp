#include <pg++/client/connect.hpp>

#include <ext/data_size.h>
#include <filesystem>

using namespace ext::literals;

namespace {
    namespace internal {
        constexpr auto default_buffer_size = 8_KiB;

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
        return connect(params, internal::default_buffer_size);
    }

    auto connect(
        const parameters& params,
        std::size_t buffer_size
    ) -> ext::task<client> {
        auto connection = std::shared_ptr<netcore::mutex<detail::connection>>(
            new netcore::mutex<detail::connection>(
                co_await internal::connect(params.host, params.port),
                buffer_size
            )
        );

        co_await connection->get().startup_message(
            params.password,
            params.params
        );

        detail::run_connection_task(connection);
        co_return pg::client(connection);
    }
}
