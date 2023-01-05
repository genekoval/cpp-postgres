#include <pg++/connection/writer.hpp>

namespace pg {
    writer::writer(netcore::socket& socket) : io(socket) {}

    auto writer::flush() -> ext::task<> {
        while (!buffer.empty()) {
            const auto bytes = co_await socket().write(
                buffer.front(),
                buffer.size()
            );

            buffer.bytes_read(bytes);
        }
    }

    auto writer::write(const void* src, std::size_t len) -> ext::task<> {
        return write_bytes(reinterpret_cast<const std::byte*>(src), len);
    }

    auto writer::write_bytes(
        const std::byte* src,
        std::size_t len
    ) -> ext::task<> {
        auto remaining = len;

        while (remaining > 0) {
            if (buffer.full()) co_await flush();

            const auto bytes = std::min(remaining, buffer.available());

            buffer.write(src, bytes);

            remaining -= bytes;
            src += bytes;
        }
    }
}
