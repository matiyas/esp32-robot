/**
 * @file test_servo_control.c
 * @brief Unit tests for servo_control component
 */

#include <unity.h>
#include "servo_control.h"

void test_servo_init(void)
{
    /* Test with NULL config */
    esp_err_t ret = servo_init(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

void test_servo_step_left(void)
{
    /* Test that step_left returns invalid state when not initialized */
    servo_cleanup();
    esp_err_t ret = servo_step_left();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
}

void test_servo_step_right(void)
{
    /* Test that step_right returns invalid state when not initialized */
    servo_cleanup();
    esp_err_t ret = servo_step_right();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
}

void test_servo_center(void)
{
    /* Test that center returns invalid state when not initialized */
    servo_cleanup();
    esp_err_t ret = servo_center();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
}
