/**
 * @file robot.c
 * @brief Robot controller facade implementation
 */

#include "robot.h"
#include "motor_control.h"
#include "servo_control.h"
#include "safety_handler.h"
#include <esp_log.h>
#include <string.h>

static const char *TAG = "robot";

/* Robot state */
static struct {
    bool initialized;
    robot_config_t config;
} s_robot = {0};

esp_err_t robot_init(const robot_config_t *config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(&s_robot.config, config, sizeof(robot_config_t));
    s_robot.initialized = true;

    ESP_LOGI(TAG, "Robot initialized (gpio_enabled=%d, movement_timeout=%lu, turret_timeout=%lu)",
             config->gpio_enabled, config->movement_timeout_ms, config->turret_timeout_ms);

    return ESP_OK;
}

robot_result_t robot_move(robot_direction_t direction, uint32_t duration_ms)
{
    robot_result_t result = {
        .success = false,
        .duration_ms = 0,
        .action = ROBOT_ACTION_STOP_ALL
    };

    if (!s_robot.initialized) {
        ESP_LOGE(TAG, "Robot not initialized");
        return result;
    }

    /* Validate and clamp duration */
    uint32_t validated_duration =
        safety_validate_duration(duration_ms, s_robot.config.movement_timeout_ms);

    if (validated_duration != duration_ms && duration_ms > 0) {
        ESP_LOGW(TAG, "Duration clamped from %lu to %lu ms",
                 duration_ms, validated_duration);
    }

    esp_err_t err = ESP_OK;

    switch (direction) {
        case ROBOT_DIR_FORWARD:
            result.action = ROBOT_ACTION_FORWARD;
            if (s_robot.config.gpio_enabled) {
                err = motor_move_forward(validated_duration);
            }
            break;

        case ROBOT_DIR_BACKWARD:
            result.action = ROBOT_ACTION_BACKWARD;
            if (s_robot.config.gpio_enabled) {
                err = motor_move_backward(validated_duration);
            }
            break;

        case ROBOT_DIR_LEFT:
            result.action = ROBOT_ACTION_LEFT;
            if (s_robot.config.gpio_enabled) {
                err = motor_turn_left(validated_duration);
            }
            break;

        case ROBOT_DIR_RIGHT:
            result.action = ROBOT_ACTION_RIGHT;
            if (s_robot.config.gpio_enabled) {
                err = motor_turn_right(validated_duration);
            }
            break;

        default:
            ESP_LOGE(TAG, "Invalid direction: %d", direction);
            return result;
    }

    if (err == ESP_OK) {
        result.success = true;
        result.duration_ms = validated_duration;

        /* Schedule auto-stop if duration specified */
        if (validated_duration > 0) {
            safety_schedule_auto_stop(validated_duration);
        }

        ESP_LOGI(TAG, "Move %s for %lu ms",
                 robot_action_to_str(result.action), validated_duration);
    } else {
        ESP_LOGE(TAG, "Move failed: %s", esp_err_to_name(err));
    }

    return result;
}

robot_result_t robot_turret(robot_direction_t direction, uint32_t duration_ms)
{
    robot_result_t result = {
        .success = false,
        .duration_ms = 0,
        .action = ROBOT_ACTION_STOP_ALL
    };

    if (!s_robot.initialized) {
        ESP_LOGE(TAG, "Robot not initialized");
        return result;
    }

    /* Validate duration (for logging, servo uses step mode) */
    uint32_t validated_duration =
        safety_validate_duration(duration_ms, s_robot.config.turret_timeout_ms);

    esp_err_t err = ESP_OK;

    switch (direction) {
        case ROBOT_DIR_LEFT:
            result.action = ROBOT_ACTION_TURRET_LEFT;
            if (s_robot.config.gpio_enabled) {
                err = servo_step_left();
            }
            break;

        case ROBOT_DIR_RIGHT:
            result.action = ROBOT_ACTION_TURRET_RIGHT;
            if (s_robot.config.gpio_enabled) {
                err = servo_step_right();
            }
            break;

        default:
            ESP_LOGE(TAG, "Invalid turret direction: %d", direction);
            return result;
    }

    if (err == ESP_OK) {
        result.success = true;
        result.duration_ms = validated_duration;
        ESP_LOGI(TAG, "Turret %s", robot_action_to_str(result.action));
    } else {
        ESP_LOGE(TAG, "Turret failed: %s", esp_err_to_name(err));
    }

    return result;
}

robot_result_t robot_stop(void)
{
    robot_result_t result = {
        .action = ROBOT_ACTION_STOP_ALL,
        .duration_ms = 0,
        .success = true
    };

    if (!s_robot.initialized) {
        ESP_LOGE(TAG, "Robot not initialized");
        result.success = false;
        return result;
    }

    /* Cancel pending auto-stop */
    safety_cancel_auto_stop();

    /* Stop motors */
    if (s_robot.config.gpio_enabled) {
        esp_err_t err = motor_stop_all();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Motor stop failed: %s", esp_err_to_name(err));
            result.success = false;
        }
    }

    ESP_LOGI(TAG, "Emergency stop");
    return result;
}

robot_status_t robot_get_status(void)
{
    robot_status_t status = {
        .connected = s_robot.initialized,
        .gpio_enabled = s_robot.config.gpio_enabled,
        .camera_url = s_robot.config.camera_url
    };

    return status;
}

void robot_cleanup(void)
{
    if (!s_robot.initialized) {
        return;
    }

    ESP_LOGI(TAG, "Robot cleanup");

    safety_cancel_auto_stop();

    if (s_robot.config.gpio_enabled) {
        motor_stop_all();
        motor_control_cleanup();
        servo_cleanup();
    }

    s_robot.initialized = false;
}
