#include "type.test.hpp"

using namespace std::literals;

namespace {
    constexpr auto test_string = "hello"sv;
}

class ByteaTest : public pg::test::TypeTest {
    template <typename T>
    auto insert(const T& t) -> pg::result {
        auto result = pg::result();

        run([&]() -> ext::task<> {
            co_await client.query("CREATE TEMP TABLE bytea_test (bytes bytea)");
            co_await client.query("INSERT INTO bytea_test VALUES ($1)", t);

            result = co_await client.query("SELECT * FROM bytea_test");
        }());

        return result;
    }
protected:
    template <typename T>
    auto test_write(const T& t) -> void {
        const auto result = insert(t);
        EXPECT_EQ(R"(\x68656c6c6f)"sv, result[0][0].string());
    }

    template <typename T>
    auto test_empty_write(const T& t) -> void {
        const auto result = insert(t);
        EXPECT_EQ(R"(\x)"sv, result[0][0].string());
    }
};

TEST_F(ByteaTest, ConstructByteaDefault) {
    const auto bytea = pg::bytea();

    EXPECT_EQ(nullptr, bytea.data());
    EXPECT_EQ(0, bytea.size());
}

TEST_F(ByteaTest, ConstructByteaSize) {
    auto bytea = pg::bytea(test_string.size());

    EXPECT_EQ(test_string.size(), bytea.size());
    EXPECT_NE(nullptr, bytea.data());

    std::memcpy(bytea.data(), test_string.data(), bytea.size());

    const auto string = std::string_view {
        reinterpret_cast<const char*>(bytea.data()),
        bytea.size()
    };

    EXPECT_EQ(test_string, string);
}

TEST_F(ByteaTest, ConstructByteaSpan) {
    const auto bytea = pg::bytea {
        std::span<const std::byte> {
            reinterpret_cast<const std::byte*>(test_string.data()),
            test_string.size()
        }
    };

    ASSERT_EQ(test_string.size(), bytea.size());

    const auto* data = reinterpret_cast<const char*>(bytea.data());

    EXPECT_EQ('h', data[0]);
    EXPECT_EQ('e', data[1]);
    EXPECT_EQ('l', data[2]);
    EXPECT_EQ('l', data[3]);
    EXPECT_EQ('o', data[4]);
}

TEST_F(ByteaTest, ConstructByteaSizeZero) {
    const auto bytea = pg::bytea(0);

    EXPECT_EQ(0, bytea.size());
    EXPECT_EQ(nullptr, bytea.data());
}

TEST_F(ByteaTest, ConstructByteaEmptySpan) {
    const auto bytea = pg::bytea(std::span<const std::byte>());

    EXPECT_EQ(0, bytea.size());
    EXPECT_EQ(nullptr, bytea.data());
}

TEST_F(ByteaTest, ReadBytea) {
    run([&]() -> ext::task<> {
        const auto result = co_await client.fetch<pg::bytea>(
            R"(SELECT '\x 68 65 6c 6c 6f'::bytea)"
        );

        EXPECT_EQ(5, result.size());
        if (result.size() != 5) co_return;

        const auto string = std::string_view(
            reinterpret_cast<const char*>(result.data()),
            result.size()
        );

        EXPECT_EQ(test_string, string);
    }());
}

TEST_F(ByteaTest, ReadByteaEmpty) {
    run([&]() -> ext::task<> {
        const auto result = co_await client.fetch<pg::bytea>(
            R"(SELECT ''::bytea)"
        );

        EXPECT_EQ(0, result.size());
        EXPECT_EQ(nullptr, result.data());
    }());
}

TEST_F(ByteaTest, WriteBytea) {
    const auto bytea = pg::bytea{
        std::span<const std::byte> {
            reinterpret_cast<const std::byte*>(test_string.data()),
            test_string.size()
        }
    };

    test_write(bytea);
}

TEST_F(ByteaTest, WriteByteaEmpty) {
    test_empty_write(pg::bytea());
}

TEST_F(ByteaTest, WriteSpan) {
    const auto bytes = std::span<const std::byte> {
        reinterpret_cast<const std::byte*>(test_string.data()),
        test_string.size()
    };

    test_write(bytes);
}

TEST_F(ByteaTest, WriteSpanEmpty) {
    test_empty_write(std::span<const std::byte>());
}
