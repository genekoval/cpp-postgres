#include <pg++/client/transaction.hpp>

#include <cassert>

namespace {
    auto rollback(
        std::shared_ptr<netcore::mutex<pg::detail::connection>> handle
    ) -> ext::detached_task {
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
    transaction::transaction(
        std::shared_ptr<netcore::mutex<detail::connection>> connection
    ) :
        handle(std::move(connection))
    {}

    transaction::transaction(transaction&& other) :
        handle(std::move(other.handle))
    {}

    transaction::~transaction() {
        if (handle && !done) ::rollback(std::move(handle));
    }

    auto transaction::operator=(transaction&& other) -> transaction& {
        handle = std::move(other.handle);
        return *this;
    }

    auto transaction::commit() -> ext::task<> {
        assert(handle);
        auto connection = co_await handle->lock();
        co_await connection->exec("COMMIT");
        done = true;

        TIMBER_DEBUG("{} commit transaction", *connection);
    }

    auto transaction::rollback() -> ext::task<> {
        assert(handle);
        auto connection = co_await handle->lock();
        co_await connection->exec("ROLLBACK");
        done = true;

        TIMBER_DEBUG("{} rollback transaction", *connection);
    }
}
