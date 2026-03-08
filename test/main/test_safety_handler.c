/**
 * @file test_safety_handler.c
 * @brief Unit tests for safety_handler component
 */

#include <unity.h>
#include "safety_handler.h"

void test_safety_validate_duration(void)
{
    /* Test zero duration */
    uint32_t result = safety_validate_duration(0, 5000);
    TEST_ASSERT_EQUAL(0, result);

    /* Test within limit */
    result = safety_validate_duration(1000, 5000);
    TEST_ASSERT_EQUAL(1000, result);

    /* Test at limit */
    result = safety_validate_duration(5000, 5000);
    TEST_ASSERT_EQUAL(5000, result);

    /* Test exceeds limit - should be clamped */
    result = safety_validate_duration(10000, 5000);
    TEST_ASSERT_EQUAL(5000, result);
}

void test_safety_auto_stop(void)
{
    /* Test that schedule returns invalid state when not initialized */
    safety_handler_cleanup();
    esp_err_t ret = safety_schedule_auto_stop(1000);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
}
