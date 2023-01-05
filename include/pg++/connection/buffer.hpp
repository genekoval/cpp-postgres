#pragma once

#include <ext/data_size.h>
#include <span>

namespace pg::detail {
    using namespace ext::literals;

    class buffer final {
        std::array<std::byte, 8_KiB> buffer;
        std::size_t head = 0;
        std::size_t tail = 0;
    public:
        auto available() const noexcept -> std::size_t;

        auto back() const noexcept -> const std::byte*;

        auto back() noexcept -> std::byte*;

        auto bytes_read(std::size_t n) noexcept -> void;

        auto bytes_written(std::size_t n) noexcept -> void;

        auto capacity() const noexcept -> std::size_t;

        auto clear() noexcept -> void;

        auto data() const noexcept -> std::span<const std::byte>;

        auto empty() const noexcept -> bool;

        auto front() const noexcept -> const std::byte*;

        auto front() noexcept -> std::byte*;

        auto full() const noexcept -> bool;

        auto read(std::byte* dest, std::size_t len) -> void;

        auto size() const noexcept -> std::size_t;

        auto write(const std::byte* src, std::size_t len) -> void;
    };
}
