#include <pg++/connection/type/byte.hpp>
#include <pg++/connection/type/error_fields.hpp>
#include <pg++/connection/type/string.hpp>

namespace pg::detail {
    auto decoder<error_fields>::decode(
        reader& reader
    ) -> ext::task<error_fields> {
        auto fields = error_fields();

        auto field = '\0';

        const auto read = [&reader]
            <typename T>
            (T& value) -> ext::task<> {
                auto string = co_await decoder<std::string>::decode(reader);

                if constexpr (std::same_as<T, std::string>) {
                    value = std::move(string);
                }
                else if constexpr (std::same_as<T, std::optional<sqlstate>>) {
                    value = parse_sqlstate(string);
                }
                else {
                    value = std::stoi(string);
                }
            };

        do {
            field = co_await decoder<char>::decode(reader);

            switch (field) {
                case 'V':
                    co_await read(fields.severity);
                    break;
                case 'C':
                    co_await read(fields.sqlstate);
                    break;
                case 'M':
                    co_await read(fields.message);
                    break;
                case 'D':
                    co_await read(fields.detail);
                    break;
                case 'H':
                    co_await read(fields.hint);
                    break;
                case 'P':
                    co_await read(fields.original_position);
                    break;
                case 'p':
                    co_await read(fields.internal_position);
                    break;
                case 'q':
                    co_await read(fields.internal_query);
                    break;
                case 'W':
                    co_await read(fields.where);
                    break;
                case 'S':
                    co_await read(fields.schema);
                    break;
                case 't':
                    co_await read(fields.table);
                    break;
                case 'c':
                    co_await read(fields.column);
                    break;
                case 'd':
                    co_await read(fields.data_type);
                    break;
                case 'n':
                    co_await read(fields.constraint);
                    break;
                case 'F':
                    co_await read(fields.file);
                    break;
                case 'L':
                    co_await read(fields.line);
                    break;
                case 'R':
                    co_await read(fields.routine);
                    break;
            }
        } while (field);

        co_return fields;
    }
}
