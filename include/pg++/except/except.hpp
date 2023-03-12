#pragma once

#include "errcodes.gen.hpp"

#include <stdexcept>

namespace pg {
    struct error : public std::runtime_error {
        error(const std::string& message);
    };

    struct bad_conversion : error {
        using error::error;
    };

    struct broken_connection : error {
        broken_connection();
    };

    struct error_fields {
        std::string column;
        std::string constraint;
        std::string data_type;
        std::string detail;
        std::string file;
        std::string hint;
        int internal_position;
        std::string internal_query;
        int line;
        std::string message;
        int original_position;
        std::string routine;
        std::string severity;
        std::string schema;
        std::optional<sqlstate> sqlstate;
        std::string table;
        std::string where;
    };

    using notice = error_fields;

    class sql_error : public error {
        error_fields _fields;
    public:
        sql_error(error_fields&& fields);

        auto fields() const noexcept -> const error_fields&;

        auto sqlstate(pg::sqlstate sqlstate) const noexcept -> bool;
    };

    struct unexpected_data : public error {
        using error::error;
    };

    struct unexpected_message : public error {
        unexpected_message(char c);
    };
}
