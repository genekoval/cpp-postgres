#pragma once

#include <pg++/connection/connection.hpp>

namespace pg {
    template <typename T>
    class portal {
        std::shared_ptr<netcore::mutex<detail::connection>> connection;
        std::string name;
        std::int32_t max_rows;
        std::vector<T> rows;
        std::optional<std::string> tag;
    public:
        portal(
            std::shared_ptr<netcore::mutex<detail::connection>>&& connection,
            std::string_view name,
            std::int32_t max_rows
        ) :
            connection(connection),
            name(name),
            max_rows(max_rows)
        {
            rows.reserve(max_rows);
        }

        explicit operator bool() const noexcept {
            return !done();
        }

        auto command_tag() const noexcept -> std::optional<std::string_view> {
            return tag;
        }

        auto done() const noexcept -> bool {
            return tag.has_value();
        }

        auto next() -> ext::task<std::span<const T>> {
            rows.clear();
            if (done()) co_return rows;

            auto connection = co_await this->connection->lock();

            tag = co_await connection->execute(
                name,
                std::back_inserter(rows),
                max_rows
            );

            if (done()) {
                co_await connection->close_portal(name);
                co_await connection->sync();
            }

            co_return rows;
        }
    };
}
