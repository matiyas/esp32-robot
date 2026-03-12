/**
 * @file test_robot.c
 * @brief Unit tests for robot_core component
 */

#include <unity.h>
#include "robot.h"

void test_robot_init(void)
{
    robot_config_t config = {
        .movement_timeout_ms = 5000,
        .turret_timeout_ms = 2000,
        .gpio_enabled = false,
        .camera_url = "/stream"
    };

    esp_err_t ret = robot_init(&config);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    robot_cleanup();
}

void test_robot_move_forward(void)
{
    robot_config_t config = {
        .movement_timeout_ms = 5000,
        .turret_timeout_ms = 2000,
        .gpio_enabled = false,
        .camera_url = "/stream"
    };

    robot_init(&config);

    robot_result_t result = robot_move(ROBOT_DIR_FORWARD, 1000);

    TEST_ASSERT_TRUE(result.success);
    TEST_ASSERT_EQUAL(ROBOT_ACTION_FORWARD, result.action);
    TEST_ASSERT_EQUAL(1000, result.duration_ms);

    robot_cleanup();
}

void test_robot_move_backward(void)
{
    robot_config_t config = {
        .movement_timeout_ms = 5000,
        .turret_timeout_ms = 2000,
        .gpio_enabled = false,
        .camera_url = "/stream"
    };

    robot_init(&config);

    robot_result_t result = robot_move(ROBOT_DIR_BACKWARD, 500);

    TEST_ASSERT_TRUE(result.success);
    TEST_ASSERT_EQUAL(ROBOT_ACTION_BACKWARD, result.action);

    robot_cleanup();
}

void test_robot_turn_left(void)
{
    robot_config_t config = {
        .movement_timeout_ms = 5000,
        .turret_timeout_ms = 2000,
        .gpio_enabled = false,
        .camera_url = "/stream"
    };

    robot_init(&config);

    robot_result_t result = robot_move(ROBOT_DIR_LEFT, 300);

    TEST_ASSERT_TRUE(result.success);
    TEST_ASSERT_EQUAL(ROBOT_ACTION_LEFT, result.action);

    robot_cleanup();
}

void test_robot_turn_right(void)
{
    robot_config_t config = {
        .movement_timeout_ms = 5000,
        .turret_timeout_ms = 2000,
        .gpio_enabled = false,
        .camera_url = "/stream"
    };

    robot_init(&config);

    robot_result_t result = robot_move(ROBOT_DIR_RIGHT, 300);

    TEST_ASSERT_TRUE(result.success);
    TEST_ASSERT_EQUAL(ROBOT_ACTION_RIGHT, result.action);

    robot_cleanup();
}

void test_robot_stop(void)
{
    robot_config_t config = {
        .movement_timeout_ms = 5000,
        .turret_timeout_ms = 2000,
        .gpio_enabled = false,
        .camera_url = "/stream"
    };

    robot_init(&config);

    robot_result_t result = robot_stop();

    TEST_ASSERT_TRUE(result.success);
    TEST_ASSERT_EQUAL(ROBOT_ACTION_STOP_ALL, result.action);

    robot_cleanup();
}

void test_robot_led_on(void)
{
    robot_config_t config = {
        .movement_timeout_ms = 5000,
        .turret_timeout_ms = 2000,
        .gpio_enabled = false,
        .camera_url = "/stream"
    };

    robot_init(&config);

    robot_result_t result = robot_led(true);

    TEST_ASSERT_TRUE(result.success);

    robot_cleanup();
}

void test_robot_led_off(void)
{
    robot_config_t config = {
        .movement_timeout_ms = 5000,
        .turret_timeout_ms = 2000,
        .gpio_enabled = false,
        .camera_url = "/stream"
    };

    robot_init(&config);

    robot_result_t result = robot_led(false);

    TEST_ASSERT_TRUE(result.success);

    robot_cleanup();
}
