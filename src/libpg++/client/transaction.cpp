#include <pg++/client/transaction.hpp>

using handle_t = std::shared_ptr<netcore::mutex<pg::detail::connection>>;

namespace {
    auto rollback(handle_t handle) -> ext::detached_task {
        try {
            auto connection = co_await handle->lock();
            co_await connection->exec("ROLLBACK");

            TIMBER_DEBUG("{} rollback transaction", *connection);
        }
        catch (const std::exception& ex) {
            TIMBER_ERROR(
                "{} rollback failed: {}",
                handle->get(),
                ex.what()
            );
        }
        catch (...) {
            TIMBER_ERROR("{} rollback failed", handle->get());
        }
    }
}

namespace pg {
    transaction::transaction(handle_t&& connection) :
        handle(std::forward<handle_t>(connection)),
        open(true)
    {}

    transaction::transaction(transaction&& other) :
        handle(std::move(other.handle)),
        open(std::exchange(other.open, false))
    {}

    transaction::~transaction() {
        if (handle && open) ::rollback(std::move(handle));
    }

    auto transaction::operator=(transaction&& other) -> transaction& {
        handle = std::move(other.handle);
        open = std::exchange(other.open, false);

        return *this;
    }

    auto transaction::commit() -> ext::task<> {
        if (handle && open) {
            auto connection = co_await handle->lock();
            co_await connection->exec("COMMIT");
            open = false;

            TIMBER_DEBUG("{} commit transaction", *connection);
        }
    }

    auto transaction::rollback() -> ext::task<> {
        if (handle && open) {
            auto connection = co_await handle->lock();
            co_await connection->exec("ROLLBACK");
            open = false;

            TIMBER_DEBUG("{} rollback transaction", *connection);
        }
    }
}
