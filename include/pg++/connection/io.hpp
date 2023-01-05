#pragma once

#include "buffer.hpp"

#include <netcore/netcore>

namespace pg {
    template <typename T>
    struct type {};
}

namespace pg::detail {
    class io {
        std::reference_wrapper<netcore::socket> socket_ref;
    protected:
        detail::buffer buffer;

        explicit io(netcore::socket& socket);

        auto socket() noexcept -> netcore::socket&;
    public:
        auto bind(netcore::socket& socket) noexcept -> void;
    };
}
