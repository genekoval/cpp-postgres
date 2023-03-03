#include <pg++/except/except.hpp>

#include <fmt/format.h>

namespace pg {
    error::error(const std::string& message) : std::runtime_error(message) {}

    broken_connection::broken_connection() :
        error("lost or failed backend connection")
    {}

    sql_error::sql_error(error_fields&& fields) :
        error(fields.message),
        _fields(std::forward<error_fields>(fields))
    {}

    auto sql_error::fields() const noexcept -> const error_fields& {
        return _fields;
    }

    unexpected_message::unexpected_message(char c) :
        error(fmt::format("received unexpected message byte {:x}", c))
    {}
}
