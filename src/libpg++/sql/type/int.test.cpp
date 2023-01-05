#include "type.test.hpp"

using IntegerTest = pg::test::TypeTest;

TEST_F(IntegerTest, Int16) {
    test_read_write<std::int16_t>(100);
}

TEST_F(IntegerTest, Int32) {
    test_read_write<std::int32_t>(100);
}

TEST_F(IntegerTest, Int64) {
    test_read_write<std::int64_t>(100);
}
