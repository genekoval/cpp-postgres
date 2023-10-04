#include <pg++/connection/socket.hpp>

namespace pg::detail {
    socket::socket(socket&& other) : inner(std::move(other.inner)) {}

    auto socket::operator=(socket&& other) -> socket& {
        inner = std::move(other.inner);
        return *this;
    }

    auto socket::cancel() noexcept -> void { return inner.cancel(); }

    auto socket::flush() -> ext::task<> { return inner.flush(); }
}
