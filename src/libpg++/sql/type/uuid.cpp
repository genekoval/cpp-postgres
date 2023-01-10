#include <pg++/sql/type/uuid.hpp>

namespace pg {
    auto type<uuid>::from_sql(
        std::int32_t size,
        reader& reader
    ) -> ext::task<uuid> {
        if (size != UUID::size) {
            throw bad_conversion(fmt::format(
                "insufficent data for uuid: expected {} bytes; received {}",
                UUID::size,
                size
            ));
        }

        auto bytes = std::array<unsigned char, UUID::size>();
        co_await reader.read(bytes.data(), bytes.size());

        co_return uuid{bytes};
    }

    auto type<uuid>::to_sql(const uuid& id, writer& writer) -> ext::task<> {
        const auto bytes = id.bytes();
        co_await writer.write(bytes.data(), bytes.size());
    }
}
