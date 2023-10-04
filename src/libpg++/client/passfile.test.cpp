#include "passfile.hpp"

#include <gtest/gtest.h>

using namespace std::literals;

using pg::detail::passfile_fields;

namespace {
    constexpr auto password = "secret"sv;

    constexpr auto test_fields = passfile_fields {
        .hostname = "localhost",
        .port = "5432",
        .database = "test",
        .username = "test"};
}

class Passfile : public testing::Test {
protected:
    auto expect_password(const std::string& text) -> void {
        expect_password(password, text);
    }

    auto expect_password(std::string_view pw, const std::string& text) -> void {
        EXPECT_EQ(pw, passfile(text).value());
    }

    auto passfile(const std::string& text) -> std::optional<std::string> {
        return passfile(test_fields, text);
    }

    auto passfile(const passfile_fields& fields, const std::string& text)
        -> std::optional<std::string> {
        auto stream = std::istringstream(text);
        return pg::detail::passfile(fields, stream);
    }
};

TEST_F(Passfile, Empty) { EXPECT_FALSE(passfile("").has_value()); }

TEST_F(Passfile, FirstLineNoNewline) {
    expect_password("localhost:5432:test:test:secret");
}

TEST_F(Passfile, FirstLineWithNewline) {
    expect_password("localhost:5432:test:test:secret\n");
}

TEST_F(Passfile, CommentsIgnored) {
    expect_password(
        "# This is a comment\n"
        "localhost:5432:test:test:secret"
    );
}

TEST_F(Passfile, EmptyLinesIgnored) {
    expect_password(
        "# This is a comment\n"
        "\n"
        "localhost:5432:test:test:secret"
    );
}

TEST_F(Passfile, SkipLine) {
    expect_password(
        "localhost:5432:test:user:foobar\n"
        "localhost:5432:test:test:secret"
    );
}

TEST_F(Passfile, Escape) {
    expect_password(
        "super:secret:password",
        R"(localhost:5432:test:test:super\:secret\:password)"
    );
}

TEST_F(Passfile, Wildcard) {
    expect_password("*:5432:test:test:secret");
    expect_password("localhost:*:test:test:secret");
    expect_password("localhost:5432:*:test:secret");
    expect_password("localhost:5432:test:*:secret");
    expect_password("*:*:*:*:secret");
}
