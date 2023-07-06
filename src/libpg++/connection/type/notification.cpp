#include <pg++/connection/type/int.hpp>
#include <pg++/connection/type/notification.hpp>
#include <pg++/connection/type/string.hpp>

namespace pg::detail {
    auto decoder<notification>::decode(
        netcore::buffered_socket& reader
    ) -> ext::task<notification> {
        auto notif = notification();

        notif.pid = co_await decoder<std::int32_t>::decode(reader);
        notif.channel = co_await decoder<std::string>::decode(reader);
        notif.payload = co_await decoder<std::string>::decode(reader);

        co_return notif;
    }
}
