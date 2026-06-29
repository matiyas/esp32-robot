/**
 * @file crsf_protocol.c
 * @brief Pure CRSF frame parser implementation (host-testable).
 */

#include "crsf_protocol.h"

uint8_t crsf_crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            if (crc & 0x80) {
                crc = (uint8_t)((crc << 1) ^ 0xD5);
            } else {
                crc = (uint8_t)(crc << 1);
            }
        }
    }
    return crc;
}

crsf_parse_status_t crsf_frame_parse(const uint8_t *buf, size_t avail, crsf_frame_t *frame,
                                     size_t *frame_total_len) {
    if (buf == NULL || frame == NULL || frame_total_len == NULL) {
        return CRSF_PARSE_INVALID;
    }

    /* Need at least the sync byte and the length byte. */
    if (avail < 2) {
        return CRSF_PARSE_INCOMPLETE;
    }

    if (buf[0] != CRSF_ADDRESS_FLIGHT_CONTROLLER) {
        return CRSF_PARSE_INVALID;
    }

    /* LEN counts TYPE + PAYLOAD + CRC. Must hold at least TYPE + CRC. */
    uint8_t len = buf[1];
    if (len < 2 || len > 62) {
        return CRSF_PARSE_INVALID;
    }

    size_t total = (size_t)len + 2; /* sync + len byte + (TYPE+PAYLOAD+CRC) */
    if (avail < total) {
        return CRSF_PARSE_INCOMPLETE;
    }

    /* CRC is over TYPE + PAYLOAD = (len - 1) bytes starting at buf[2]. */
    uint8_t expected = crsf_crc8(&buf[2], (size_t)(len - 1));
    uint8_t actual = buf[total - 1];
    if (expected != actual) {
        return CRSF_PARSE_INVALID;
    }

    frame->type = buf[2];
    frame->payload = &buf[3];
    frame->payload_len = (uint8_t)(len - 2);
    *frame_total_len = total;
    return CRSF_PARSE_OK;
}

void crsf_unpack_channels(const uint8_t *payload, uint16_t *out) {
    if (payload == NULL || out == NULL) {
        return;
    }

    uint32_t bits = 0;
    int nbits = 0;
    const uint8_t *p = payload;

    for (int ch = 0; ch < CRSF_NUM_CHANNELS; ch++) {
        while (nbits < 11) {
            bits |= (uint32_t)(*p++) << nbits;
            nbits += 8;
        }
        out[ch] = (uint16_t)(bits & 0x07FF);
        bits >>= 11;
        nbits -= 11;
    }
}
