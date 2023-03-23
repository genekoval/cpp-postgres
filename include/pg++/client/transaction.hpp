#pragma once

#include <pg++/connection/connection.hpp>

namespace pg {
    class transaction {
        std::shared_ptr<netcore::mutex<detail::connection>> handle;
        bool open = false;
    public:
        transaction() = default;

        explicit transaction(
            std::shared_ptr<netcore::mutex<detail::connection>>&& connection
        );

        transaction(const transaction&) = delete;

        transaction(transaction&& other);

        ~transaction();

        auto operator=(const transaction&) -> transaction& = delete;

        auto operator=(transaction&& other) -> transaction&;

        auto commit() -> ext::task<>;

        auto rollback() -> ext::task<>;
    };
}
