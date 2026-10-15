#include "test_framework.h"

#include <cstdint>

#include "save_integrity.h"

using engine::save::crc32;
using engine::save::SAVE_MAGIC;
using engine::save::SAVE_VERSION;
using engine::save::SaveHeader;
using engine::save::validate_checksum;
using engine::save::validate_header;

TEST_CASE(crc32_of_empty_input_is_zero) {
    CHECK_EQ(crc32(nullptr, 0), 0u);
}

TEST_CASE(crc32_matches_known_ieee_test_vector) {
    // "123456789" is the standard CRC-32 (IEEE 802.3, poly 0xEDB88320)
    // conformance test vector - same algorithm/polynomial this table-free
    // implementation uses, so this checks against an external reference,
    // not just internal self-consistency.
    const auto* data = reinterpret_cast<const uint8_t*>("123456789");
    CHECK_EQ(crc32(data, 9), 0xCBF43926u);
}

TEST_CASE(crc32_is_sensitive_to_every_byte) {
    const uint8_t a[] = {1, 2, 3, 4};
    const uint8_t b[] = {1, 2, 3, 5};
    CHECK(crc32(a, 4) != crc32(b, 4));
}

TEST_CASE(validate_header_rejects_wrong_magic) {
    SaveHeader header{};
    header.magic = 0xDEADBEEF;
    header.version = 1;
    CHECK(!validate_header(header));
}

TEST_CASE(validate_header_rejects_version_zero) {
    SaveHeader header{};
    header.magic = SAVE_MAGIC;
    header.version = 0;
    CHECK(!validate_header(header));
}

TEST_CASE(validate_header_rejects_version_beyond_current) {
    SaveHeader header{};
    header.magic = SAVE_MAGIC;
    header.version = static_cast<uint16_t>(SAVE_VERSION + 1);
    CHECK(!validate_header(header));
}

TEST_CASE(validate_header_accepts_current_version) {
    SaveHeader header{};
    header.magic = SAVE_MAGIC;
    header.version = SAVE_VERSION;
    CHECK(validate_header(header));
}

TEST_CASE(validate_checksum_rejects_empty_payload) {
    SaveHeader header{};
    uint8_t payload[4] = {1, 2, 3, 4};
    header.checksum = crc32(payload, 4);
    CHECK(!validate_checksum(header, payload, 0));
}

TEST_CASE(validate_checksum_rejects_mismatch) {
    SaveHeader header{};
    uint8_t payload[4] = {1, 2, 3, 4};
    header.checksum = crc32(payload, 4) ^ 1u;  // flip one bit
    CHECK(!validate_checksum(header, payload, 4));
}

TEST_CASE(validate_checksum_accepts_matching_payload) {
    SaveHeader header{};
    uint8_t payload[4] = {1, 2, 3, 4};
    header.checksum = crc32(payload, 4);
    CHECK(validate_checksum(header, payload, 4));
}