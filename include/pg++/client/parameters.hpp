#pragma once

#include <pg++/connection/type/parameter_list.hpp>

#include <optional>
#include <string>

namespace pg {
    struct parameters {
        std::string host;
        std::optional<std::string> port;
        std::string password;
        parameter_list params;
    };
}
