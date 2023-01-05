#include "type.test.hpp"

using StringTest = pg::test::TypeTest;

TEST_F(StringTest, String) {
    test_read_write<std::string>("Hello, World!");
}

TEST_F(StringTest, StringView) {
    test_write<std::string>(std::string_view("Foo Bar"));
}

TEST_F(StringTest, StringLiteral) {
    test_write<std::string>("PostgreSQL");
}
