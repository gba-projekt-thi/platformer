#include "test_framework.h"

#include <cstdint>

#include "save_buffer.h"

using engine::save::SaveBuffer;

TEST_CASE(save_buffer_write_read_roundtrip_preserves_values_and_order) {
    SaveBuffer<64> buf;
    const int a = 42;
    const float b = 3.5f;
    const uint8_t c = 200;

    CHECK(buf.write(a));
    CHECK(buf.write(b));
    CHECK(buf.write(c));
    CHECK_EQ(buf.size, static_cast<int>(sizeof(a) + sizeof(b) + sizeof(c)));

    int a2 = 0;
    float b2 = 0;
    uint8_t c2 = 0;
    CHECK(buf.read(a2));
    CHECK(buf.read(b2));
    CHECK(buf.read(c2));

    CHECK_EQ(a2, a);
    CHECK_EQ(b2, b);
    CHECK_EQ(c2, c);
}

TEST_CASE(save_buffer_write_fails_once_full) {
    SaveBuffer<4> buf;
    const int a = 1;
    CHECK(buf.write(a));  // exactly fills the 4-byte buffer

    const int b = 2;
    CHECK(!buf.write(b));  // no room left
    CHECK_EQ(buf.size, 4);
}

TEST_CASE(save_buffer_read_fails_past_end) {
    SaveBuffer<64> buf;
    const int a = 1;
    buf.write(a);

    int out1 = 0;
    int out2 = 0;
    CHECK(buf.read(out1));   // consumes the 4 written bytes
    CHECK(!buf.read(out2));  // nothing left
}

TEST_CASE(save_buffer_write_bytes_and_read_bytes_roundtrip) {
    SaveBuffer<32> buf;
    const uint8_t src[5] = {10, 20, 30, 40, 50};
    CHECK(buf.write_bytes(src, 5));

    uint8_t dst[5] = {};
    CHECK(buf.read_bytes(dst, 5));
    for (int i = 0; i < 5; ++i) {
        CHECK_EQ(dst[i], src[i]);
    }
}

TEST_CASE(save_buffer_clear_resets_write_and_read_positions) {
    SaveBuffer<64> buf;
    int a = 1;
    buf.write(a);
    buf.read(a);

    buf.clear();
    CHECK(buf.empty());
    CHECK_EQ(buf.size, 0);
    CHECK_EQ(buf.read_offset, 0);
}

TEST_CASE(save_buffer_reset_read_rewinds_without_clearing_data) {
    SaveBuffer<64> buf;
    int a = 7;
    buf.write(a);

    int out = 0;
    buf.read(out);
    CHECK_EQ(out, 7);

    buf.reset_read();
    out = 0;
    CHECK(buf.read(out));
    CHECK_EQ(out, 7);
}

TEST_CASE(save_buffer_remaining_and_capacity_track_writes) {
    SaveBuffer<64> buf;
    CHECK_EQ(buf.capacity(), 64);
    CHECK_EQ(buf.remaining(), 64);

    const int a = 1;
    buf.write(a);
    CHECK_EQ(buf.remaining(), 64 - static_cast<int>(sizeof(a)));
}