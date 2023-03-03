#include <pg++/sql/type/int.hpp>

#include <ext/bit>

#define INT_IMPL(Type) \
    auto pg::type<Type>::from_sql( \
        std::int32_t size, \
        pg::reader& reader \
    ) -> ext::task<Type> { \
        if (size != sizeof(Type)) { \
            throw bad_conversion(fmt::format(\
                "unexpected data for int{}: expected {} bytes; received {}", \
                sizeof(Type) * 8, \
                sizeof(Type), \
                size \
            )); \
        } \
        Type result = 0; \
        co_await reader.read(&result, sizeof(Type)); \
        TIMBER_TRACE("read SQL int{}: {}", sizeof(Type) * 8, result); \
        co_return ext::from_be(result); \
    } \
    \
    auto pg::type<Type>::to_sql( \
        Type i, \
        pg::writer& writer \
    ) -> ext::task<> { \
        const auto be = ext::to_be(i); \
        co_await writer.write(&be, sizeof(Type)); \
    } \
    \
    auto pg::type<Type>::size(Type i) -> std::int32_t { \
        return sizeof(Type); \
    }

INT_IMPL(std::int16_t);
INT_IMPL(std::int32_t);
INT_IMPL(std::int64_t);
