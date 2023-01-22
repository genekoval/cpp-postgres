#pragma once

#include <pg++/connection/type/parameter_list.hpp>

#include <string>

namespace pg {
    struct parameters {
        static auto get() -> parameters;

        static auto parse(const parameter_list& params) -> parameters;

        std::string host;
        std::string port;
        std::string password;
        parameter_list params;
    };
}
