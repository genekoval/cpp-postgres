#include <pg++/connection/socket.hpp>

namespace pg::detail {
    socket::socket(socket&& other) :
        inner(std::move(other.inner)),
        reader(std::move(other.reader)),
        writer(std::move(other.writer))
    {
        reader.bind(inner);
        writer.bind(inner);
    }

    auto socket::operator=(socket&& other) -> socket& {
        inner = std::move(other.inner);

        reader = std::move(other.reader);
        reader.bind(inner);

        writer = std::move(other.writer);
        writer.bind(inner);

        return *this;
    }

    auto socket::cancel() noexcept -> void {
        return inner.cancel();
    }

    auto socket::flush() -> ext::task<> {
        return writer.flush();
    }
}
