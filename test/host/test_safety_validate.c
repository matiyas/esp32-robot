/**
 * @file test_safety_validate.c
 * @brief Host-side unit tests for safety validation
 */

#include "unity.h"
#include <stdint.h>

/* Function under test */
extern uint32_t safety_validate_duration(uint32_t duration_ms, uint32_t max_duration_ms);

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

    return UNITY_END();
}
