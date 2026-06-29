/**
 * @file rc_map.c
 * @brief Pure CRSF-channel-to-control mapping implementation (host-testable).
 */

#include "rc_map.h"

#include "crsf_protocol.h"

static int clamp_int(int value, int lo, int hi) {
    if (value < lo) {
        return lo;
    }
    if (value > hi) {
        return hi;
    }
    return value;
}

int8_t rc_map_throttle(uint16_t tick, const rc_throttle_cfg_t *cfg) {
    int center = cfg->center_tick ? (int)cfg->center_tick : CRSF_TICK_MID;
    int db = (int)cfg->deadband_ticks;
    int max_duty = clamp_int((int)cfg->max_duty, 0, 100);
    int duty;

    if ((int)tick > center + db) {
        int span = CRSF_TICK_MAX - center - db;
        if (span < 1) {
            span = 1;
        }
        duty = ((int)tick - center - db) * max_duty / span;
    } else if ((int)tick < center - db) {
        int span = center - db - CRSF_TICK_MIN;
        if (span < 1) {
            span = 1;
        }
        duty = -(((center - db) - (int)tick) * max_duty / span);
    } else {
        duty = 0;
    }

    if (cfg->reverse) {
        duty = -duty;
    }

    return (int8_t)clamp_int(duty, -max_duty, max_duty);
}

uint8_t rc_map_steering(uint16_t tick, const rc_steer_cfg_t *cfg) {
    int center_tick = cfg->center_tick ? (int)cfg->center_tick : CRSF_TICK_MID;
    int max_angle = (int)cfg->max_angle_deg;
    int trimmed_center = (int)cfg->servo_center_deg + (int)cfg->center_trim_deg;
    int offset;

    if ((int)tick >= center_tick) {
        int span = CRSF_TICK_MAX - center_tick;
        if (span < 1) {
            span = 1;
        }
        offset = ((int)tick - center_tick) * max_angle / span;
    } else {
        int span = center_tick - CRSF_TICK_MIN;
        if (span < 1) {
            span = 1;
        }
        offset = -(((center_tick - (int)tick) * max_angle) / span);
    }

    if (cfg->reverse) {
        offset = -offset;
    }

    int angle =
        clamp_int(trimmed_center + offset, trimmed_center - max_angle, trimmed_center + max_angle);
    return (uint8_t)clamp_int(angle, 0, 180);
}

bool rc_is_armed(uint16_t tick) {
    return (int)tick > CRSF_TICK_MID;
}

bool rc_link_is_active(int64_t last_frame_us, int64_t now_us, uint32_t timeout_ms) {
    if (last_frame_us <= 0) {
        return false;
    }
    return (now_us - last_frame_us) < (int64_t)timeout_ms * 1000;
}
