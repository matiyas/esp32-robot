/**
 * @file test_rc_map.c
 * @brief Host-side unit tests for the pure CRSF control mapping.
 */

#include "unity.h"

#include "crsf_protocol.h"
#include "rc_map.h"

void test_throttle_center_is_zero(void) {
    rc_throttle_cfg_t cfg = {.center_tick = 0, .deadband_ticks = 20, .max_duty = 100, .reverse = false};
    TEST_ASSERT_EQUAL_INT8(0, rc_map_throttle(CRSF_TICK_MID, &cfg));
    TEST_ASSERT_EQUAL_INT8(0, rc_map_throttle(CRSF_TICK_MID + 10, &cfg)); /* inside deadband */
    TEST_ASSERT_EQUAL_INT8(0, rc_map_throttle(CRSF_TICK_MID - 10, &cfg));
}

void test_throttle_full_range(void) {
    rc_throttle_cfg_t cfg = {.center_tick = 0, .deadband_ticks = 20, .max_duty = 100, .reverse = false};
    TEST_ASSERT_EQUAL_INT8(100, rc_map_throttle(CRSF_TICK_MAX, &cfg));
    int8_t rev = rc_map_throttle(CRSF_TICK_MIN, &cfg);
    TEST_ASSERT_TRUE(rev <= -99); /* full reverse within rounding */
    TEST_ASSERT_TRUE(rev >= -100);
}

void test_throttle_max_duty_cap(void) {
    rc_throttle_cfg_t cfg = {.center_tick = 0, .deadband_ticks = 20, .max_duty = 50, .reverse = false};
    TEST_ASSERT_EQUAL_INT8(50, rc_map_throttle(CRSF_TICK_MAX, &cfg));
    int8_t rev = rc_map_throttle(CRSF_TICK_MIN, &cfg);
    TEST_ASSERT_TRUE(rev >= -50 && rev <= -49);
}

void test_throttle_reverse_flag(void) {
    rc_throttle_cfg_t cfg = {.center_tick = 0, .deadband_ticks = 20, .max_duty = 100, .reverse = true};
    TEST_ASSERT_EQUAL_INT8(-100, rc_map_throttle(CRSF_TICK_MAX, &cfg));
}

void test_throttle_deadband_edge_continuous(void) {
    rc_throttle_cfg_t cfg = {.center_tick = 0, .deadband_ticks = 20, .max_duty = 100, .reverse = false};
    /* Just past the deadband should be a small, non-saturated value. */
    int8_t just_past = rc_map_throttle(CRSF_TICK_MID + 21, &cfg);
    TEST_ASSERT_TRUE(just_past >= 0 && just_past < 5);
}

void test_steering_center_and_trim(void) {
    rc_steer_cfg_t cfg = {.center_tick = 0,
                          .center_trim_deg = 0,
                          .max_angle_deg = 45,
                          .servo_center_deg = 90,
                          .reverse = false};
    TEST_ASSERT_EQUAL_UINT8(90, rc_map_steering(CRSF_TICK_MID, &cfg));

    cfg.center_trim_deg = 5;
    TEST_ASSERT_EQUAL_UINT8(95, rc_map_steering(CRSF_TICK_MID, &cfg));
}

void test_steering_endpoints_clamped(void) {
    rc_steer_cfg_t cfg = {.center_tick = 0,
                          .center_trim_deg = 0,
                          .max_angle_deg = 45,
                          .servo_center_deg = 90,
                          .reverse = false};
    TEST_ASSERT_EQUAL_UINT8(135, rc_map_steering(CRSF_TICK_MAX, &cfg));
    TEST_ASSERT_EQUAL_UINT8(45, rc_map_steering(CRSF_TICK_MIN, &cfg));
}

void test_steering_reverse(void) {
    rc_steer_cfg_t cfg = {.center_tick = 0,
                          .center_trim_deg = 0,
                          .max_angle_deg = 45,
                          .servo_center_deg = 90,
                          .reverse = true};
    TEST_ASSERT_EQUAL_UINT8(45, rc_map_steering(CRSF_TICK_MAX, &cfg));
    TEST_ASSERT_EQUAL_UINT8(135, rc_map_steering(CRSF_TICK_MIN, &cfg));
}

void test_arm_threshold(void) {
    TEST_ASSERT_FALSE(rc_is_armed(CRSF_TICK_MIN));
    TEST_ASSERT_FALSE(rc_is_armed(CRSF_TICK_MID));
    TEST_ASSERT_TRUE(rc_is_armed(CRSF_TICK_MID + 1));
    TEST_ASSERT_TRUE(rc_is_armed(CRSF_TICK_MAX));
}

void test_link_liveness(void) {
    TEST_ASSERT_FALSE(rc_link_is_active(0, 1000000, 500));        /* never seen */
    TEST_ASSERT_TRUE(rc_link_is_active(1000000, 1100000, 500));   /* 100ms old */
    TEST_ASSERT_FALSE(rc_link_is_active(1000000, 1600000, 500));  /* 600ms old */
}
