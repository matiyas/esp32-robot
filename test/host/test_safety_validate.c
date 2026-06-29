/**
 * @file test_safety_validate.c
 * @brief Host-side unit tests for safety validation
 */

#include "unity.h"
#include <stdint.h>

/* Function under test */
extern uint32_t safety_validate_duration(uint32_t duration_ms, uint32_t max_duration_ms);

/* CRSF parser tests (test_crsf_parser.c) */
extern void test_crsf_crc8_known_vectors(void);
extern void test_crsf_parse_valid_frame(void);
extern void test_crsf_parse_rejects_bad_crc(void);
extern void test_crsf_parse_incomplete(void);
extern void test_crsf_parse_rejects_bad_sync_and_len(void);
extern void test_crsf_unpack_endianness(void);

/* CRSF mapping tests (test_rc_map.c) */
extern void test_throttle_center_is_zero(void);
extern void test_throttle_full_range(void);
extern void test_throttle_max_duty_cap(void);
extern void test_throttle_reverse_flag(void);
extern void test_throttle_deadband_edge_continuous(void);
extern void test_steering_center_and_trim(void);
extern void test_steering_endpoints_clamped(void);
extern void test_steering_reverse(void);
extern void test_arm_threshold(void);
extern void test_link_liveness(void);

void setUp(void) {}
void tearDown(void) {}

void test_validate_zero_duration(void)
{
    uint32_t result = safety_validate_duration(0, 5000);
    TEST_ASSERT_EQUAL_UINT32(0, result);
}

void test_validate_within_limit(void)
{
    uint32_t result = safety_validate_duration(1000, 5000);
    TEST_ASSERT_EQUAL_UINT32(1000, result);
}

void test_validate_at_limit(void)
{
    uint32_t result = safety_validate_duration(5000, 5000);
    TEST_ASSERT_EQUAL_UINT32(5000, result);
}

void test_validate_exceeds_limit(void)
{
    uint32_t result = safety_validate_duration(10000, 5000);
    TEST_ASSERT_EQUAL_UINT32(5000, result);
}

void test_validate_large_values(void)
{
    uint32_t result = safety_validate_duration(UINT32_MAX, 5000);
    TEST_ASSERT_EQUAL_UINT32(5000, result);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_validate_zero_duration);
    RUN_TEST(test_validate_within_limit);
    RUN_TEST(test_validate_at_limit);
    RUN_TEST(test_validate_exceeds_limit);
    RUN_TEST(test_validate_large_values);

    RUN_TEST(test_crsf_crc8_known_vectors);
    RUN_TEST(test_crsf_parse_valid_frame);
    RUN_TEST(test_crsf_parse_rejects_bad_crc);
    RUN_TEST(test_crsf_parse_incomplete);
    RUN_TEST(test_crsf_parse_rejects_bad_sync_and_len);
    RUN_TEST(test_crsf_unpack_endianness);

    RUN_TEST(test_throttle_center_is_zero);
    RUN_TEST(test_throttle_full_range);
    RUN_TEST(test_throttle_max_duty_cap);
    RUN_TEST(test_throttle_reverse_flag);
    RUN_TEST(test_throttle_deadband_edge_continuous);
    RUN_TEST(test_steering_center_and_trim);
    RUN_TEST(test_steering_endpoints_clamped);
    RUN_TEST(test_steering_reverse);
    RUN_TEST(test_arm_threshold);
    RUN_TEST(test_link_liveness);

    return UNITY_END();
}
