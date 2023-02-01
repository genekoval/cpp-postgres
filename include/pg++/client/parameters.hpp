#pragma once

#include <pg++/connection/type/parameter_list.hpp>

#include <string>

namespace pg {
    struct parameters {
        static auto get() -> parameters;

        template <typename Map>
        requires
            std::same_as<
                typename Map::value_type,
                parameter_list::value_type
            > &&
            requires(const Map& map) {
                { map.begin() } -> std::input_iterator;
                { map.end() } -> std::input_iterator;
            }
        static auto parse(const Map& map) -> parameters {
            return parse(parameter_list(map.begin(), map.end()));
        }

        static auto parse(parameter_list&& params) -> parameters;

        std::string host;
        std::string port;
        std::string password;
        parameter_list params;
    };
}
