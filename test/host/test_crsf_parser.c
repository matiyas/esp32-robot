/**
 * @file test_crsf_parser.c
 * @brief Host-side unit tests for the pure CRSF frame parser.
 */

#include "unity.h"

#include "crsf_protocol.h"

#include <string.h>

/* Pack 16 channels x 11 bits little-endian into a 22-byte payload. */
static void pack_channels(const uint16_t in[CRSF_NUM_CHANNELS], uint8_t out[CRSF_RC_PAYLOAD_BYTES]) {
    memset(out, 0, CRSF_RC_PAYLOAD_BYTES);
    int bitpos = 0;
    for (int ch = 0; ch < CRSF_NUM_CHANNELS; ch++) {
        uint32_t v = (uint32_t)(in[ch] & 0x07FF);
        for (int b = 0; b < 11; b++) {
            if (v & (1u << b)) {
                out[bitpos / 8] |= (uint8_t)(1u << (bitpos % 8));
            }
            bitpos++;
        }
    }
}

/* Build a complete RC_CHANNELS_PACKED frame; returns total length (26). */
static size_t build_rc_frame(const uint16_t ch[CRSF_NUM_CHANNELS], uint8_t *buf) {
    buf[0] = CRSF_ADDRESS_FLIGHT_CONTROLLER;
    buf[1] = 0x18; /* LEN = type(1) + payload(22) + crc(1) = 24 */
    buf[2] = CRSF_FRAMETYPE_RC_CHANNELS_PACKED;
    pack_channels(ch, &buf[3]);
    buf[25] = crsf_crc8(&buf[2], 1 + CRSF_RC_PAYLOAD_BYTES); /* over TYPE+PAYLOAD */
    return 26;
}

void test_crsf_crc8_known_vectors(void) {
    uint8_t zero = 0x00;
    uint8_t one = 0x01;
    TEST_ASSERT_EQUAL_UINT8(0x00, crsf_crc8(&zero, 0)); /* empty */
    TEST_ASSERT_EQUAL_UINT8(0x00, crsf_crc8(&zero, 1)); /* {0x00} */
    TEST_ASSERT_EQUAL_UINT8(0xD5, crsf_crc8(&one, 1));  /* {0x01} -> poly */
}

void test_crsf_parse_valid_frame(void) {
    uint16_t ch[CRSF_NUM_CHANNELS];
    for (int i = 0; i < CRSF_NUM_CHANNELS; i++) {
        ch[i] = CRSF_TICK_MID;
    }
    uint8_t buf[64];
    size_t total = build_rc_frame(ch, buf);

    crsf_frame_t frame;
    size_t consumed = 0;
    TEST_ASSERT_EQUAL_INT(CRSF_PARSE_OK, crsf_frame_parse(buf, total, &frame, &consumed));
    TEST_ASSERT_EQUAL_size_t(26, consumed);
    TEST_ASSERT_EQUAL_UINT8(CRSF_FRAMETYPE_RC_CHANNELS_PACKED, frame.type);
    TEST_ASSERT_EQUAL_UINT8(CRSF_RC_PAYLOAD_BYTES, frame.payload_len);
}

void test_crsf_parse_rejects_bad_crc(void) {
    uint16_t ch[CRSF_NUM_CHANNELS];
    for (int i = 0; i < CRSF_NUM_CHANNELS; i++) {
        ch[i] = CRSF_TICK_MID;
    }
    uint8_t buf[64];
    size_t total = build_rc_frame(ch, buf);
    buf[10] ^= 0xFF; /* corrupt a payload byte */

    crsf_frame_t frame;
    size_t consumed = 0;
    TEST_ASSERT_EQUAL_INT(CRSF_PARSE_INVALID, crsf_frame_parse(buf, total, &frame, &consumed));
}

void test_crsf_parse_incomplete(void) {
    uint16_t ch[CRSF_NUM_CHANNELS];
    for (int i = 0; i < CRSF_NUM_CHANNELS; i++) {
        ch[i] = CRSF_TICK_MID;
    }
    uint8_t buf[64];
    build_rc_frame(ch, buf);

    crsf_frame_t frame;
    size_t consumed = 0;
    TEST_ASSERT_EQUAL_INT(CRSF_PARSE_INCOMPLETE, crsf_frame_parse(buf, 5, &frame, &consumed));
    TEST_ASSERT_EQUAL_INT(CRSF_PARSE_INCOMPLETE, crsf_frame_parse(buf, 1, &frame, &consumed));
}

void test_crsf_parse_rejects_bad_sync_and_len(void) {
    uint8_t bad_sync[4] = {0x00, 0x18, 0x16, 0x00};
    uint8_t bad_len[4] = {CRSF_ADDRESS_FLIGHT_CONTROLLER, 0x01, 0x16, 0x00};
    crsf_frame_t frame;
    size_t consumed = 0;
    TEST_ASSERT_EQUAL_INT(CRSF_PARSE_INVALID, crsf_frame_parse(bad_sync, 4, &frame, &consumed));
    TEST_ASSERT_EQUAL_INT(CRSF_PARSE_INVALID, crsf_frame_parse(bad_len, 4, &frame, &consumed));
}

void test_crsf_unpack_endianness(void) {
    uint16_t in[CRSF_NUM_CHANNELS];
    for (int i = 0; i < CRSF_NUM_CHANNELS; i++) {
        in[i] = CRSF_TICK_MID;
    }
    in[0] = CRSF_TICK_MIN; /* 172 */
    in[1] = CRSF_TICK_MAX; /* 1811 */
    in[15] = 1234;

    uint8_t payload[CRSF_RC_PAYLOAD_BYTES];
    pack_channels(in, payload);

    uint16_t out[CRSF_NUM_CHANNELS];
    crsf_unpack_channels(payload, out);
    for (int i = 0; i < CRSF_NUM_CHANNELS; i++) {
        TEST_ASSERT_EQUAL_UINT16(in[i], out[i]);
    }
}
