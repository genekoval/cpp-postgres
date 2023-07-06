#pragma once

#include "int.hpp"

#include <pg++/sql/sql.hpp>

namespace pg::detail {
    template <pg::from_sql T>
    auto from_sql(netcore::buffered_socket& reader) -> ext::task<T> {
        using type = type<std::remove_cvref_t<T>>;

        const auto size =
            co_await detail::decoder<std::int32_t>::decode(reader);

        if (size == -1) {
            if constexpr (nullable<T>) co_return type::null();
            else {
                if constexpr (pg::has_typname<T>) {
                    throw bad_conversion(fmt::format(
                        "received NULL when value of type {} was expected",
                        type::name
                    ));
                }
                else {
                    bad_conversion("type does not support NULL");
                }
            }
        }

        co_return co_await type::from_sql(size, reader);
    }

    template <to_sql T>
    class sql_type {
        std::reference_wrapper<const T> value;
    public:
        sql_type(const T& value) : value(value) {}

        auto get() const noexcept -> const T& {
            return value.get();
        }
    };

    template <to_sql T>
    struct encoder<sql_type<T>> {
        static auto encode(
            sql_type<T> t,
            netcore::buffered_socket& writer
        ) -> ext::task<> {
            using size_encoder = detail::encoder<std::int32_t>;

            const auto& value = t.get();

            if constexpr (nullable<T>) {
                if (type<T>::is_null(value)) {
                    co_await size_encoder::encode(-1, writer);
                    co_return;
                }
            }

            co_await size_encoder::encode(type<T>::size(value), writer);
            co_await type<T>::to_sql(value, writer);
        }

        static auto size(sql_type<T> t) -> std::int32_t {
            constexpr std::int32_t self = sizeof(std::int32_t);

            if constexpr (nullable<T>) {
                if (type<T>::is_null(t.get())) return self;
            }

            return self + type<T>::size(t.get());
        }
    };
}
