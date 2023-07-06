#include <pg++/sql/type/uuid.hpp>

namespace pg {
    auto type<uuid>::from_sql(
        std::int32_t size,
        netcore::buffered_socket& reader
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

        auto result = uuid(bytes);

        TIMBER_TRACE("read SQL uuid: {}", result);

        co_return result;
    }

    auto type<uuid>::to_sql(
        const uuid& id,
        netcore::buffered_socket& writer
    ) -> ext::task<> {
        const auto bytes = id.bytes();
        co_await writer.write(bytes.data(), bytes.size());
    }
}
