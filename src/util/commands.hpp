#pragma once

#include <commline/commline>

namespace pg::cmd {
    auto dump() -> std::unique_ptr<commline::command_node>;

    auto oid() -> std::unique_ptr<commline::command_node>;
}
