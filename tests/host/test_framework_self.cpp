#include "test_framework.h"

#include <cstdint>
#include <string>

// The harness itself: what a failing CHECK_EQ prints and which tests a
// filter selects.

namespace {

enum class Color : uint8_t { Red, Green = 7 };

}  // namespace

TEST_CASE(framework_prints_integers) {
    CHECK_EQ(testfw::to_text(42), std::string("42"));
    CHECK_EQ(testfw::to_text(-7), std::string("-7"));
    CHECK_EQ(testfw::to_text(uint16_t{65535}), std::string("65535"));
    CHECK_EQ(testfw::to_text(uint32_t{4000000000u}), std::string("4000000000"));
}

TEST_CASE(framework_prints_bools_as_words) {
    CHECK_EQ(testfw::to_text(true), std::string("true"));
    CHECK_EQ(testfw::to_text(false), std::string("false"));
}

TEST_CASE(framework_prints_enums_by_their_value) {
    CHECK_EQ(testfw::to_text(Color::Green), std::string("7"));
}

TEST_CASE(framework_prints_chars_and_strings_quoted) {
    CHECK_EQ(testfw::to_text('a'), std::string("'a'"));
    CHECK_EQ(testfw::to_text("dash"), std::string("\"dash\""));
    CHECK_EQ(
        testfw::to_text(static_cast<const char*>(nullptr)),
        std::string("nullptr"));
}

TEST_CASE(framework_prints_floating_point) {
    CHECK_EQ(testfw::to_text(0.5), std::string("0.5"));
}

TEST_CASE(framework_prints_fixed_point_types_by_their_value) {
    struct Fixed {
        [[nodiscard]] auto to_double() const -> double { return 1.25; }
    };
    CHECK_EQ(testfw::to_text(Fixed{}), std::string("1.25"));
}

TEST_CASE(framework_prints_unknown_types_as_a_placeholder) {
    struct Opaque {};
    CHECK_EQ(testfw::to_text(Opaque{}), std::string("<?>"));
}

TEST_CASE(framework_empty_filter_selects_every_test) {
    CHECK(testfw::selected("crumble_gives_way", nullptr));
    CHECK(testfw::selected("crumble_gives_way", ""));
}

TEST_CASE(framework_filter_selects_by_substring) {
    CHECK(testfw::selected("crumble_gives_way", "crumble"));
    CHECK(testfw::selected("crumble_gives_way", "gives"));
    CHECK(!testfw::selected("dash_starts", "crumble"));
}
