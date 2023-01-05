#include <pg++/connection/io.hpp>

namespace pg::detail {
    io::io(netcore::socket& socket) : socket_ref(socket) {}

    auto io::bind(netcore::socket& socket) noexcept -> void {
        socket_ref = socket;
    }

    auto io::socket() noexcept -> netcore::socket& {
        return socket_ref;
    }
}
