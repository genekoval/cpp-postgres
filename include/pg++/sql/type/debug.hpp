#pragma once

#include "../sql.hpp"

namespace pg::debug {
    class bytes {
        std::unique_ptr<std::byte[]> ptr;
        std::size_t count;
        std::string str;
    public:
        bytes() = default;

        bytes(std::unique_ptr<std::byte[]>&& data, std::size_t size) :
            ptr(std::forward<std::unique_ptr<std::byte[]>>(data)),
            count(size)
        {
            auto buffer = fmt::memory_buffer();

            for (auto i = 0u; i < count; ++i) {
                fmt::format_to(
                    std::back_inserter(buffer),
                    "{:0>2x}",
                    static_cast<unsigned char>(ptr[i])
                );

                if (i != count - 1) {
                    fmt::format_to(std::back_inserter(buffer), " ");
                }
            }

            str = fmt::to_string(buffer);
        }

        bytes(const bytes&) = delete;

        bytes(bytes&& other) = default;

        auto operator=(const bytes&) -> bytes& = delete;

        auto operator=(bytes&& other) -> bytes& = default;

        auto data() const noexcept -> std::span<const std::byte> {
            return {ptr.get(), count};
        }

        auto get() const noexcept -> const std::byte* {
            return ptr.get();
        }

        auto size() const noexcept -> std::size_t {
            return count;
        }

        auto string() const noexcept -> std::string_view {
            return str;
        }
    };
}

namespace pg {
    template <>
    struct type<debug::bytes> {
        static auto from_sql(
            std::int32_t size,
            netcore::buffered_socket& reader
        ) -> ext::task<debug::bytes> {
            auto data = std::unique_ptr<std::byte[]>(new std::byte[size]);
            co_await reader.read(data.get(), size);
            co_return debug::bytes(std::move(data), size);
        }
    };

    static_assert(from_sql<debug::bytes>);
}

template <>
struct fmt::formatter<pg::debug::bytes> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(
        const pg::debug::bytes& bytes,
        FormatContext& ctx
    ) {
        return format_to(
            ctx.out(),
            "({:L} byte{}) {}",
            bytes.size(),
            bytes.size() == 1 ? "" : "s",
            bytes.string()
        );
    }
};
