#include <pg++/sql/type/bytea.hpp>

namespace pg {
    bytea::bytea(std::size_t size) :
        bytes(size == 0 ? nullptr : new std::byte[size]),
        count(size)
    {}

    bytea::bytea(std::span<const std::byte> bytes) :
        bytes(bytes.empty() ? nullptr : new std::byte[bytes.size()]),
        count(bytes.size())
    {
        if (bytes.empty()) return;
        std::memcpy(this->bytes.get(), bytes.data(), bytes.size());
    }

    auto bytea::data() noexcept -> std::byte* {
        return bytes.get();
    }

    auto bytea::data() const noexcept -> const std::byte* {
        return bytes.get();
    }

    auto bytea::size() const noexcept -> std::size_t {
        return count;
    }

    auto bytea::span() noexcept -> std::span<std::byte> {
        return {bytes.get(), count};
    }

    auto bytea::span() const noexcept -> std::span<const std::byte> {
        return {bytes.get(), count};
    }

    auto type<bytea>::from_sql(
        std::int32_t size,
        reader& reader
    ) -> ext::task<bytea> {
        auto result = bytea{static_cast<std::size_t>(size)};
        co_await reader.read(result.data(), result.size());
        co_return result;
    }

    auto type<bytea>::to_sql(
        const bytea& bytea,
        writer& writer
    ) -> ext::task<> {
        co_await writer.write(bytea.data(), bytea.size());
    }

    auto type<bytea>::size(const bytea& bytea) -> std::int32_t {
        return bytea.size();
    }

    auto type<std::span<const std::byte>>::to_sql(
        std::span<const std::byte> bytes,
        writer& writer
    ) -> ext::task<> {
        co_await writer.write(bytes.data(), bytes.size());
    }

    auto type<std::span<const std::byte>>::size(
        std::span<const std::byte> bytes
    ) -> std::int32_t {
        return bytes.size();
    }
}
