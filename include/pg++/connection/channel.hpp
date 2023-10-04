#pragma once

#include <memory>
#include <netcore/netcore>
#include <unordered_map>
#include <unordered_set>

namespace pg::detail {
    class connection;

    class channel final {
        std::string channel_name;
        netcore::event<std::string> event;
        std::unordered_set<std::int32_t> ignored;
        std::weak_ptr<netcore::mutex<detail::connection>> connection;
    public:
        channel() = default;

        channel(
            std::string_view name,
            std::weak_ptr<netcore::mutex<detail::connection>>&& connection
        );

        ~channel();

        auto close() -> void;

        auto ignore(std::int32_t pid) -> void;

        auto listen() -> ext::task<std::string>;

        auto name() const noexcept -> std::string_view;

        auto notify(const std::string& payload, std::int32_t pid) -> void;

        auto unignore(std::int32_t pid) -> void;
    };
}

namespace pg {
    class channel final {
        std::shared_ptr<detail::channel> inner;
    public:
        channel() = default;

        channel(std::shared_ptr<detail::channel>&& inner);

        auto ignore(std::int32_t pid) -> void;

        auto listen() -> ext::task<std::string>;

        auto name() const noexcept -> std::string_view;

        auto unignore(std::int32_t pid) -> void;
    };
}
