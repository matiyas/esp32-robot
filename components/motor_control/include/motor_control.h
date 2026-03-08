/**
 * @file motor_control.h
 * @brief DRV8833 motor driver control interface
 *
 * Controls two DC motors via DRV8833 H-bridge drivers with
 * shared PWM enable for soft-start ramping.
 */

#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include "robot_types.h"
#include <esp_err.h>
#include <driver/gpio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Motor pin configuration
 */
typedef struct {
    gpio_num_t in1;  /**< Forward direction pin */
    gpio_num_t in2;  /**< Backward direction pin */
} motor_pins_t;

/**
 * @brief Motor control configuration
 */
typedef struct {
    motor_pins_t left_motor;       /**< Left motor pins */
    motor_pins_t right_motor;      /**< Right motor pins */
    gpio_num_t enable_pin;         /**< Shared PWM enable pin */
    uint32_t pwm_frequency_hz;     /**< PWM frequency (default: 1000) */
    uint32_t ramp_duration_ms;     /**< Soft-start ramp time (default: 500) */
    uint8_t ramp_steps;            /**< Number of ramp steps (default: 25) */
} motor_control_config_t;

/**
 * @brief Initialize motor control subsystem
 *
 * Configures GPIO pins and PWM for motor control.
 *
 * @param config Motor configuration
 * @return ESP_OK on success
 */
esp_err_t motor_control_init(const motor_control_config_t *config);

/**
 * @brief Move forward (both motors forward)
 *
 * @param duration_ms Duration (0 for continuous)
 * @return ESP_OK on success
 */
esp_err_t motor_move_forward(uint32_t duration_ms);

/**
 * @brief Move backward (both motors backward)
 *
 * @param duration_ms Duration (0 for continuous)
 * @return ESP_OK on success
 */
esp_err_t motor_move_backward(uint32_t duration_ms);

/**
 * @brief Tank turn left (left backward, right forward)
 *
 * @param duration_ms Duration (0 for continuous)
 * @return ESP_OK on success
 */
esp_err_t motor_turn_left(uint32_t duration_ms);

/**
 * @brief Tank turn right (left forward, right backward)
 *
 * @param duration_ms Duration (0 for continuous)
 * @return ESP_OK on success
 */
esp_err_t motor_turn_right(uint32_t duration_ms);

/**
 * @brief Stop all motors (coast mode)
 *
 * Sets all motor pins to LOW for freewheeling stop.
 *
 * @return ESP_OK on success
 */
esp_err_t motor_stop_all(void);

/**
 * @brief Cleanup motor control resources
 */
void motor_control_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* MOTOR_CONTROL_H */
