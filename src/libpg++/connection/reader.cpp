#include <pg++/connection/reader.hpp>
#include <pg++/except/except.hpp>

namespace pg {
    reader::reader(netcore::socket& socket) : io(socket) {}

    auto reader::advance(std::size_t n) noexcept -> void {
        buffer.bytes_read(n);
    }

    auto reader::data() -> ext::task<std::span<const std::byte>> {
        if (buffer.empty()) co_await fill();
        co_return buffer.data();
    }

    auto reader::fill() -> ext::task<> {
        const auto bytes = co_await socket().read(
            buffer.back(),
            buffer.available()
        );

        if (bytes == 0) throw broken_connection();

        buffer.bytes_written(bytes);
    }

    auto reader::read(void* dest, std::size_t len) -> ext::task<> {
        return read_bytes(reinterpret_cast<std::byte*>(dest), len);
    }

    auto reader::read_bytes(std::byte* dest, std::size_t len) -> ext::task<> {
        auto remaining = len;

        while (remaining > 0) {
            if (buffer.empty()) co_await fill();

            const auto bytes = std::min(remaining, buffer.size());

            buffer.read(dest, bytes);

            remaining -= bytes;
            dest += bytes;
        }
    }
}
