/**
 * @file rc_map.h
 * @brief Pure CRSF-channel-to-control mapping (host-testable)
 *
 * Converts raw CRSF channel ticks into a signed motor duty and a steering
 * servo angle, plus arm/liveness helpers. No ESP/RTOS dependencies so the
 * mapping math can be unit tested on the host.
 */

#ifndef RC_MAP_H
#define RC_MAP_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Throttle mapping configuration.
 */
typedef struct {
    uint16_t center_tick;    /**< Stick-center tick (0 = use CRSF_TICK_MID). */
    uint16_t deadband_ticks; /**< Ticks around center mapped to zero. */
    uint8_t max_duty;        /**< Duty (%) at full stick, 0-100. */
    bool reverse;            /**< Invert forward/reverse. */
} rc_throttle_cfg_t;

/**
 * @brief Steering mapping configuration.
 */
typedef struct {
    uint16_t center_tick;     /**< Stick-center tick (0 = use CRSF_TICK_MID). */
    int8_t center_trim_deg;   /**< Trim added to the servo center. */
    uint8_t max_angle_deg;    /**< Deflection (deg) at full stick. */
    uint8_t servo_center_deg; /**< Servo angle at stick center (e.g. 90). */
    bool reverse;             /**< Invert left/right. */
} rc_steer_cfg_t;

/**
 * @brief Map a throttle tick to a signed duty in [-100, 100].
 *
 * Each half of the stick is scaled independently; the deadband is removed from
 * both numerator and denominator so the response is continuous at its edge.
 * The result is clamped to +/- max_duty.
 */
int8_t rc_map_throttle(uint16_t tick, const rc_throttle_cfg_t *cfg);

/**
 * @brief Map a steering tick to a servo angle in degrees.
 *
 * Linear about center, offset by trim, clamped to +/- max_angle around the
 * trimmed center and to the physical [0, 180] range.
 */
uint8_t rc_map_steering(uint16_t tick, const rc_steer_cfg_t *cfg);

/**
 * @brief Treat a (switch) channel as armed when above mid-stick.
 */
bool rc_is_armed(uint16_t tick);

/**
 * @brief True while a frame was seen within @p timeout_ms.
 *
 * @param last_frame_us Timestamp (us) of the last valid frame (<=0 = never).
 * @param now_us Current timestamp (us).
 * @param timeout_ms Liveness window in milliseconds.
 */
bool rc_link_is_active(int64_t last_frame_us, int64_t now_us, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* RC_MAP_H */
