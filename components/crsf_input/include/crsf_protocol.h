/**
 * @file crsf_protocol.h
 * @brief Pure CRSF (Crossfire / ExpressLRS) frame parser
 *
 * Decoupled from UART/RTOS so it can be unit tested on the host. Only depends
 * on the C standard library. Implements frame validation, CRC8 and the packed
 * 11-bit RC channel unpack.
 */

#ifndef CRSF_PROTOCOL_H
#define CRSF_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Flight-controller destination address / frame sync byte. */
#define CRSF_ADDRESS_FLIGHT_CONTROLLER 0xC8

/** Frame types we care about. */
#define CRSF_FRAMETYPE_RC_CHANNELS_PACKED 0x16
#define CRSF_FRAMETYPE_LINK_STATISTICS    0x14
#define CRSF_FRAMETYPE_BATTERY_SENSOR     0x08

/** RC channel tick range (matches ~1000/1500/2000us). */
#define CRSF_TICK_MIN 172
#define CRSF_TICK_MID 992
#define CRSF_TICK_MAX 1811

/** Number of channels in a packed RC frame and its payload size. */
#define CRSF_NUM_CHANNELS     16
#define CRSF_RC_PAYLOAD_BYTES 22

/** Maximum bytes a frame may occupy on the wire (sync + len + 62). */
#define CRSF_MAX_FRAME_LEN 64

/**
 * @brief Result of attempting to parse one frame from a byte buffer.
 */
typedef enum {
    CRSF_PARSE_OK = 0,     /**< A complete, CRC-valid frame is present. */
    CRSF_PARSE_INCOMPLETE, /**< Not enough bytes yet; wait for more. */
    CRSF_PARSE_INVALID     /**< Bad sync/length/CRC; drop a byte and resync. */
} crsf_parse_status_t;

/**
 * @brief A validated frame view (points into the caller's buffer).
 */
typedef struct {
    uint8_t type;           /**< Frame TYPE byte. */
    const uint8_t *payload; /**< Pointer to PAYLOAD (excludes TYPE and CRC). */
    uint8_t payload_len;    /**< PAYLOAD length in bytes (= LEN - 2). */
} crsf_frame_t;

/**
 * @brief Compute the CRSF CRC8 (poly 0xD5, init 0x00, non-reflected).
 *
 * The CRSF CRC is computed over TYPE + PAYLOAD only.
 *
 * @param data Pointer to the bytes to checksum.
 * @param len Number of bytes.
 * @return CRC8 value.
 */
uint8_t crsf_crc8(const uint8_t *data, size_t len);

/**
 * @brief Try to parse one frame starting at the front of @p buf.
 *
 * Expects @p buf[0] to be the sync/address byte. OOB-safe: never reads past
 * @p avail bytes.
 *
 * @param buf Byte buffer (frame is expected to start at index 0).
 * @param avail Number of valid bytes available in @p buf.
 * @param[out] frame Filled on CRSF_PARSE_OK.
 * @param[out] frame_total_len Bytes consumed by the frame on CRSF_PARSE_OK.
 * @return Parse status.
 */
crsf_parse_status_t crsf_frame_parse(const uint8_t *buf, size_t avail, crsf_frame_t *frame,
                                     size_t *frame_total_len);

/**
 * @brief Unpack 16 channels of 11 bits each (little-endian bit packing).
 *
 * @param payload Pointer to the 22-byte RC channels payload.
 * @param[out] out Array of at least CRSF_NUM_CHANNELS entries (raw ticks).
 */
void crsf_unpack_channels(const uint8_t *payload, uint16_t *out);

#ifdef __cplusplus
}
#endif

#endif /* CRSF_PROTOCOL_H */
