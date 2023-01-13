#include <pg++/client/client.hpp>

namespace pg {
    client::client(std::shared_ptr<detail::connection>&& connection) :
        connection(
            std::forward<std::shared_ptr<detail::connection>>(connection)
        )
    {}

    auto client::make_function_query(
        std::string_view function,
        int arg_count
    ) -> std::string {
        auto os = std::ostringstream();

        os << "SELECT * FROM " << function << "(";

        for (auto i = 1; i <= arg_count; ++i) {
            os << "$" << i;

            if (i < arg_count) os << ", ";
        }

        os << ")";

        return os.str();
    }

    auto client::on_notice(notice_callback_type&& callback) -> void {
        connection->on_notice(std::forward<notice_callback_type>(callback));
    }

    auto client::simple_query(
        std::string_view query
    ) -> ext::task<std::vector<result>> {
        return connection->query(query);
    }
}
