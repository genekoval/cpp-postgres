#include "../examples.hpp"

namespace example {
    auto simple_query(pg::client& client) -> ext::task<> {
        const auto results = co_await client.simple_query(
            "DROP TABLE IF EXISTS widget;"
            "CREATE TABLE widget (id int4, name text);"
            "INSERT INTO widget (id, name) VALUES (42, 'bar');"
            "SELECT * FROM widget;"
        );

        for (const auto& result : results) {
            if (!result.command_tag().starts_with("SELECT")) {
                fmt::print("{}\n", result.command_tag());
                continue;
            }

            for (const auto& row : result) {
                fmt::print("\n");

                for (const auto& field : row) {
                    const auto value = field.string().value_or("[null]");

                    fmt::print("{}: {}\n", field.name(), value);
                }
            }

            fmt::print(
                "\n({:L} row{})\n",
                result.size(),
                result.size() == 1 ? "" : "s"
            );
        }
    }
}
