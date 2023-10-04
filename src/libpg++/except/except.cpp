#include <pg++/except/except.hpp>

#include <fmt/format.h>

namespace {
    auto format_error(const pg::error_fields& fields) -> std::string {
        auto buffer = fmt::memory_buffer();
        auto out = std::back_inserter(buffer);

        fmt::format_to(out, "{}:  {}", fields.severity, fields.message);

        if (!fields.internal_query.empty()) {
            fmt::format_to(out, "\nQUERY:  {}", fields.internal_query);
        }

        if (!fields.where.empty()) {
            fmt::format_to(out, "\nCONTEXT:  {}", fields.where);
        }

        if (!fields.detail.empty()) {
            fmt::format_to(out, "\nDETAIL:  {}", fields.detail);
        }

        if (!fields.hint.empty()) {
            fmt::format_to(out, "\nHINT:  {}", fields.hint);
        }

        return fmt::to_string(buffer);
    }
}

namespace pg {
    error::error(const std::string& message) : std::runtime_error(message) {}

    broken_connection::broken_connection() :
        error("lost or failed backend connection") {}

    sql_error::sql_error(error_fields&& fields) :
        error(format_error(fields)),
        _fields(std::forward<error_fields>(fields)) {}

    auto sql_error::fields() const noexcept -> const error_fields& {
        return _fields;
    }

    auto sql_error::sqlstate(pg::sqlstate sqlstate) const noexcept -> bool {
        return _fields.sqlstate && *_fields.sqlstate == sqlstate;
    }

    unexpected_message::unexpected_message(char c) :
        error(fmt::format("received unexpected message byte {:x}", c)) {}
}
