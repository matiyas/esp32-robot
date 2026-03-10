/**
 * @file motor_control.c
 * @brief DRV8833 motor driver implementation
 */

#include "motor_control.h"

#include <esp_log.h>

#include <driver/gpio.h>

#include <string.h>

#include "hal_gpio.h"
#include "hal_pwm.h"
#include "pwm_ramper.h"

static const char *TAG = "motor_control";

/* Motor state */
static struct {
    bool initialized;
    motor_control_config_t config;
    hal_pwm_channel_t pwm_channel;
} s_motor = {0};

/**
 * @brief Log current GPIO states for debugging
 */
static void log_gpio_states(void) {
    ESP_LOGI(TAG, "GPIO states: IN1_L(12)=%d IN2_L(13)=%d IN1_R(14)=%d IN2_R(15)=%d",
             gpio_get_level(s_motor.config.left_motor.in1),
             gpio_get_level(s_motor.config.left_motor.in2),
             gpio_get_level(s_motor.config.right_motor.in1),
             gpio_get_level(s_motor.config.right_motor.in2));
}

/**
 * @brief Set motor direction using DRV8833 truth table
 */
static void set_motor_direction(const motor_pins_t *motor, motor_mode_t mode) {
    switch (mode) {
        case MOTOR_MODE_FORWARD:
            hal_gpio_set_level(motor->in1, 1);
            hal_gpio_set_level(motor->in2, 0);
            break;
        case MOTOR_MODE_BACKWARD:
            hal_gpio_set_level(motor->in1, 0);
            hal_gpio_set_level(motor->in2, 1);
            break;
        case MOTOR_MODE_BRAKE:
            hal_gpio_set_level(motor->in1, 1);
            hal_gpio_set_level(motor->in2, 1);
            break;
        case MOTOR_MODE_COAST:
        default:
            hal_gpio_set_level(motor->in1, 0);
            hal_gpio_set_level(motor->in2, 0);
            break;
    }
}

esp_err_t motor_control_init(const motor_control_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;

    /* Initialize left motor GPIO */
    ret = hal_gpio_init_output(config->left_motor.in1);
    if (ret != ESP_OK)
        return ret;

    ret = hal_gpio_init_output(config->left_motor.in2);
    if (ret != ESP_OK)
        return ret;

    /* Initialize right motor GPIO */
    ret = hal_gpio_init_output(config->right_motor.in1);
    if (ret != ESP_OK)
        return ret;

    ret = hal_gpio_init_output(config->right_motor.in2);
    if (ret != ESP_OK)
        return ret;

    /* Initialize PWM for enable pin */
    ret = hal_pwm_init(config->enable_pin, config->pwm_frequency_hz, &s_motor.pwm_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "PWM init failed");
        return ret;
    }

    /* Initialize PWM ramper */
    pwm_ramper_config_t ramper_config = {.ramp_duration_ms = config->ramp_duration_ms,
                                         .num_steps = config->ramp_steps,
                                         .max_duty_percent = 100};
    ret = pwm_ramper_init(s_motor.pwm_channel, &ramper_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "PWM ramper init failed");
        return ret;
    }

    memcpy(&s_motor.config, config, sizeof(motor_control_config_t));
    s_motor.initialized = true;

    /* Set motors to coast mode initially */
    motor_stop_all();

    ESP_LOGI(TAG, "Motor control initialized (freq=%lu Hz, ramp=%lu ms)", config->pwm_frequency_hz,
             config->ramp_duration_ms);

    return ESP_OK;
}

esp_err_t motor_move_forward(uint32_t duration_ms) {
    if (!s_motor.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Moving forward (duration=%lu ms)", duration_ms);

    set_motor_direction(&s_motor.config.left_motor, MOTOR_MODE_FORWARD);
    set_motor_direction(&s_motor.config.right_motor, MOTOR_MODE_FORWARD);
    log_gpio_states();
    pwm_ramper_start();

    return ESP_OK;
}

esp_err_t motor_move_backward(uint32_t duration_ms) {
    if (!s_motor.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Moving backward (duration=%lu ms)", duration_ms);

    set_motor_direction(&s_motor.config.left_motor, MOTOR_MODE_BACKWARD);
    set_motor_direction(&s_motor.config.right_motor, MOTOR_MODE_BACKWARD);
    log_gpio_states();
    pwm_ramper_start();

    return ESP_OK;
}

esp_err_t motor_turn_left(uint32_t duration_ms) {
    if (!s_motor.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Turning left (duration=%lu ms)", duration_ms);

    /* Tank turn: left backward, right forward */
    set_motor_direction(&s_motor.config.left_motor, MOTOR_MODE_BACKWARD);
    set_motor_direction(&s_motor.config.right_motor, MOTOR_MODE_FORWARD);
    log_gpio_states();
    pwm_ramper_start();

    return ESP_OK;
}

esp_err_t motor_turn_right(uint32_t duration_ms) {
    if (!s_motor.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Turning right (duration=%lu ms)", duration_ms);

    /* Tank turn: left forward, right backward */
    set_motor_direction(&s_motor.config.left_motor, MOTOR_MODE_FORWARD);
    set_motor_direction(&s_motor.config.right_motor, MOTOR_MODE_BACKWARD);
    log_gpio_states();
    pwm_ramper_start();

    return ESP_OK;
}

esp_err_t motor_stop_all(void) {
    if (!s_motor.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Stopping all motors");

    /* Stop PWM ramp */
    pwm_ramper_stop();

    /* Set both motors to coast mode */
    set_motor_direction(&s_motor.config.left_motor, MOTOR_MODE_COAST);
    set_motor_direction(&s_motor.config.right_motor, MOTOR_MODE_COAST);
    log_gpio_states();

    return ESP_OK;
}

void motor_control_cleanup(void) {
    if (!s_motor.initialized) {
        return;
    }

    ESP_LOGI(TAG, "Motor control cleanup");

    motor_stop_all();
    pwm_ramper_cleanup();
    hal_pwm_cleanup(s_motor.pwm_channel);

    /* Reset GPIO pins */
    gpio_num_t pins[] = {s_motor.config.left_motor.in1, s_motor.config.left_motor.in2,
                         s_motor.config.right_motor.in1, s_motor.config.right_motor.in2};
    hal_gpio_reset_multiple(pins, 4);

    s_motor.initialized = false;
}
