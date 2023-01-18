#include <pg++/connection/type/int.hpp>
#include <pg++/connection/type/notification.hpp>
#include <pg++/connection/type/string.hpp>

namespace pg::detail {
    auto decoder<notification>::decode(
        reader& reader
    ) -> ext::task<notification> {
        co_return notification {
            .pid = co_await decoder<std::int32_t>::decode(reader),
            .channel = co_await decoder<std::string>::decode(reader),
            .payload = co_await decoder<std::string>::decode(reader)
        };
    }
}
