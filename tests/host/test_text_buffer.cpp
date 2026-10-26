#include "test_framework.h"

#include <string>

#include "text_buffer.h"

namespace {

template <int N>
auto text(const TextBuffer<N>& buffer) -> std::string {
    return buffer.c_str();
}

}  // namespace

TEST_CASE(text_buffer_starts_empty) {
    TextBuffer<8> buffer;
    CHECK_EQ(text(buffer), std::string(""));
    CHECK_EQ(buffer.size(), 0);
}

TEST_CASE(text_buffer_appends_text_and_chars) {
    TextBuffer<16> buffer;
    buffer.append("Slot ").append('3');
    CHECK_EQ(text(buffer), std::string("Slot 3"));
    CHECK_EQ(buffer.size(), 6);
}

TEST_CASE(text_buffer_appends_unsigned_numbers) {
    TextBuffer<32> buffer;
    buffer.append_uint(0).append(' ').append_uint(7).append(' ').append_uint(
        4294967295u);
    CHECK_EQ(text(buffer), std::string("0 7 4294967295"));
}

TEST_CASE(text_buffer_pads_numbers_to_a_width) {
    TextBuffer<16> buffer;
    buffer.append_uint(5, 2).append(':').append_uint(123, 2);
    // Width is a minimum: a longer number is not cut.
    CHECK_EQ(text(buffer), std::string("05:123"));
}

TEST_CASE(text_buffer_formats_a_frame_time) {
    TextBuffer<16> buffer;
    buffer.append_time(FrameTime{1, 23, 45});
    CHECK_EQ(text(buffer), std::string("01:23.45"));
}

TEST_CASE(text_buffer_time_minutes_wrap_at_100_like_the_hud) {
    // Level best times show two minute digits (LevelSelectScene, the "New
    // Best!" banner); a record over 99 minutes wraps instead of shifting the
    // layout.
    TextBuffer<16> buffer;
    buffer.append_time(FrameTime{123, 4, 5});
    CHECK_EQ(text(buffer), std::string("23:04.05"));
}

TEST_CASE(text_buffer_never_writes_past_its_capacity) {
    TextBuffer<6> buffer;  // 5 characters + terminator
    buffer.append("abc").append("defgh").append('x').append_uint(42);
    CHECK_EQ(text(buffer), std::string("abcde"));
    CHECK_EQ(buffer.size(), 5);
}

TEST_CASE(text_buffer_cuts_a_number_at_the_capacity) {
    TextBuffer<4> buffer;
    buffer.append_uint(12345);
    CHECK_EQ(text(buffer), std::string("123"));
}

TEST_CASE(text_buffer_ignores_a_null_text) {
    TextBuffer<8> buffer;
    buffer.append(static_cast<const char*>(nullptr)).append("ok");
    CHECK_EQ(text(buffer), std::string("ok"));
}

TEST_CASE(text_buffer_clear_starts_over) {
    TextBuffer<8> buffer;
    buffer.append("abc");
    buffer.clear();
    buffer.append("x");
    CHECK_EQ(text(buffer), std::string("x"));
}
