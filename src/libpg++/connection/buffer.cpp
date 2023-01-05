#include <pg++/connection/buffer.hpp>

#include <cassert>
#include <cstring>

namespace pg::detail {
    auto buffer::available() const noexcept -> std::size_t {
        return buffer.size() - tail;
    }

    auto buffer::back() const noexcept -> const std::byte* {
        return &buffer[tail];
    }

    auto buffer::back() noexcept -> std::byte* {
        return &buffer[tail];
    }

    auto buffer::bytes_read(std::size_t n) noexcept -> void {
        assert(head + n <= tail);
        head += n;
        if (head == tail) clear();
    }

    auto buffer::bytes_written(std::size_t n) noexcept -> void {
        assert(tail + n <= buffer.size());
        tail += n;
    }

    auto buffer::capacity() const noexcept -> std::size_t {
        return buffer.size();
    }

    auto buffer::clear() noexcept -> void {
        head = 0;
        tail = 0;
    }

    auto buffer::data() const noexcept -> std::span<const std::byte> {
        return {front(), size()};
    }

    auto buffer::empty() const noexcept -> bool {
        return size() == 0;
    }

    auto buffer::front() const noexcept -> const std::byte* {
        return &buffer[head];
    }

    auto buffer::front() noexcept -> std::byte* {
        return &buffer[head];
    }

    auto buffer::full() const noexcept -> bool {
        return available() == 0;
    }

    auto buffer::read(std::byte* dest, std::size_t len) -> void {
        assert(len <= size());

        std::memcpy(dest, front(), len);
        bytes_read(len);
    }

    auto buffer::size() const noexcept -> std::size_t {
        return tail - head;
    }

    auto buffer::write(const std::byte* src, std::size_t len) -> void {
        assert(len <= available());

        std::memcpy(back(), src, len);
        bytes_written(len);
    }
}
