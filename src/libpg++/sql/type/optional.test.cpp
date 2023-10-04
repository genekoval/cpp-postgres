#include "type.test.hpp"

using OptionalTest = pg::test::TypeTest;

TEST_F(OptionalTest, Null) { test_read_write(std::optional<std::string>()); }

TEST_F(OptionalTest, NotNull) {
    test_read_write(std::optional<std::string>("hello"));
}
