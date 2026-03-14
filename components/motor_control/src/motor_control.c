/**
 * @file motor_control.c
 * @brief DRV8833 motor driver implementation with direct PWM on motor pins
 */

#include "motor_control.h"

#include <esp_log.h>

#include <driver/gpio.h>

#include <string.h>

#include "hal_gpio.h"
#include "hal_pwm.h"

static const char *TAG = "motor_control";

/* Motor state */
static struct {
    bool initialized;
    motor_control_config_t config;
    hal_pwm_channel_t left_in1_pwm;
    hal_pwm_channel_t left_in2_pwm;
    hal_pwm_channel_t right_in1_pwm;
    hal_pwm_channel_t right_in2_pwm;
} s_motor = {0};

/**
 * @brief Set motor speed and direction using PWM
 *
 * DRV8833 truth table:
 * - Forward:  IN1=PWM, IN2=LOW  (speed controlled by PWM duty)
 * - Backward: IN1=LOW, IN2=PWM  (speed controlled by PWM duty)
 * - Brake:    IN1=HIGH, IN2=HIGH
 * - Coast:    IN1=LOW, IN2=LOW
 */
static void set_motor(hal_pwm_channel_t in1, hal_pwm_channel_t in2, motor_mode_t mode,
                      uint8_t speed) {
    switch (mode) {
        case MOTOR_MODE_FORWARD:
            hal_pwm_set_duty(in1, speed);
            hal_pwm_set_duty(in2, 0);
            break;
        case MOTOR_MODE_BACKWARD:
            hal_pwm_set_duty(in1, 0);
            hal_pwm_set_duty(in2, speed);
            break;
        case MOTOR_MODE_BRAKE:
            hal_pwm_set_duty(in1, 100);
            hal_pwm_set_duty(in2, 100);
            break;
        case MOTOR_MODE_COAST:
        default:
            hal_pwm_set_duty(in1, 0);
            hal_pwm_set_duty(in2, 0);
            break;
    }
}

esp_err_t motor_control_init(const motor_control_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;

    /* Initialize PWM for all motor pins */
    ret = hal_pwm_init(config->left_motor.in1, config->pwm_frequency_hz, &s_motor.left_in1_pwm);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Left IN1 PWM init failed");
        return ret;
    }

    ret = hal_pwm_init(config->left_motor.in2, config->pwm_frequency_hz, &s_motor.left_in2_pwm);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Left IN2 PWM init failed");
        return ret;
    }

    ret = hal_pwm_init(config->right_motor.in1, config->pwm_frequency_hz, &s_motor.right_in1_pwm);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Right IN1 PWM init failed");
        return ret;
    }

    ret = hal_pwm_init(config->right_motor.in2, config->pwm_frequency_hz, &s_motor.right_in2_pwm);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Right IN2 PWM init failed");
        return ret;
    }

    /* DRV8833 nSLEEP pin should be tied to VCC to keep it always enabled */

    memcpy(&s_motor.config, config, sizeof(motor_control_config_t));
    s_motor.initialized = true;

    /* Set motors to coast mode initially */
    motor_stop_all();

    ESP_LOGI(TAG, "Motor control initialized with direct PWM (freq=%lu Hz)",
             config->pwm_frequency_hz);

    return ESP_OK;
}

esp_err_t motor_move_forward(uint32_t duration_ms) {
    if (!s_motor.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Moving forward");

    set_motor(s_motor.left_in1_pwm, s_motor.left_in2_pwm, MOTOR_MODE_FORWARD, 100);
    set_motor(s_motor.right_in1_pwm, s_motor.right_in2_pwm, MOTOR_MODE_FORWARD, 100);

    return ESP_OK;
}

esp_err_t motor_move_backward(uint32_t duration_ms) {
    if (!s_motor.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Moving backward");

    set_motor(s_motor.left_in1_pwm, s_motor.left_in2_pwm, MOTOR_MODE_BACKWARD, 100);
    set_motor(s_motor.right_in1_pwm, s_motor.right_in2_pwm, MOTOR_MODE_BACKWARD, 100);

    return ESP_OK;
}

esp_err_t motor_turn_left(uint32_t duration_ms) {
    if (!s_motor.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Turning left");

    /* Tank turn: left backward, right forward */
    set_motor(s_motor.left_in1_pwm, s_motor.left_in2_pwm, MOTOR_MODE_BACKWARD, 100);
    set_motor(s_motor.right_in1_pwm, s_motor.right_in2_pwm, MOTOR_MODE_FORWARD, 100);

    return ESP_OK;
}

esp_err_t motor_turn_right(uint32_t duration_ms) {
    if (!s_motor.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Turning right");

    /* Tank turn: left forward, right backward */
    set_motor(s_motor.left_in1_pwm, s_motor.left_in2_pwm, MOTOR_MODE_FORWARD, 100);
    set_motor(s_motor.right_in1_pwm, s_motor.right_in2_pwm, MOTOR_MODE_BACKWARD, 100);

    return ESP_OK;
}

esp_err_t motor_stop_all(void) {
    if (!s_motor.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Stopping all motors");

    set_motor(s_motor.left_in1_pwm, s_motor.left_in2_pwm, MOTOR_MODE_COAST, 0);
    set_motor(s_motor.right_in1_pwm, s_motor.right_in2_pwm, MOTOR_MODE_COAST, 0);

    return ESP_OK;
}

void motor_control_cleanup(void) {
    if (!s_motor.initialized) {
        return;
    }

    ESP_LOGI(TAG, "Motor control cleanup");

    motor_stop_all();

    hal_pwm_cleanup(s_motor.left_in1_pwm);
    hal_pwm_cleanup(s_motor.left_in2_pwm);
    hal_pwm_cleanup(s_motor.right_in1_pwm);
    hal_pwm_cleanup(s_motor.right_in2_pwm);

    s_motor.initialized = false;
}
