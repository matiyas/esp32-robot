/**
 * @file test_motor_control.c
 * @brief Unit tests for motor_control component
 */

#include <unity.h>
#include "motor_control.h"

void test_motor_control_init(void)
{
    /* Test with NULL config */
    esp_err_t ret = motor_control_init(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

void test_motor_direction_forward(void)
{
    /* Test that forward returns invalid state when not initialized */
    motor_control_cleanup();
    esp_err_t ret = motor_move_forward(1000);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
}

void test_motor_direction_backward(void)
{
    /* Test that backward returns invalid state when not initialized */
    motor_control_cleanup();
    esp_err_t ret = motor_move_backward(1000);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
}

void test_motor_stop(void)
{
    /* Test that stop returns invalid state when not initialized */
    motor_control_cleanup();
    esp_err_t ret = motor_stop_all();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
}
