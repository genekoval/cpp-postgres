#include <pg++/client/client.hpp>

namespace pg {
    client::client(std::shared_ptr<detail::connection>&& connection) :
        connection(
            std::forward<std::shared_ptr<detail::connection>>(connection)
        )
    {}

    auto client::on_notice(notice_callback_type&& callback) -> void {
        connection->on_notice(std::forward<notice_callback_type>(callback));
    }

    auto client::simple_query(
        std::string_view query
    ) -> ext::task<std::vector<result>> {
        return connection->query(query);
    }
}
