/**
 * @file servo_control.h
 * @brief SG90 servo controller interface
 *
 * Controls SG90 micro servo for turret positioning with
 * smooth interpolated movement.
 */

#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H

#include <esp_err.h>
#include <driver/gpio.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Servo configuration
 */
typedef struct {
    gpio_num_t signal_pin;         /**< PWM signal GPIO */
    uint16_t min_pulse_us;         /**< Min pulse width (default: 500) */
    uint16_t max_pulse_us;         /**< Max pulse width (default: 2400) */
    uint8_t min_angle;             /**< Min angle degrees (default: 0) */
    uint8_t max_angle;             /**< Max angle degrees (default: 180) */
    uint8_t default_angle;         /**< Default position (default: 90) */
    uint8_t step_angle;            /**< Step per command (default: 10) */
    uint8_t smooth_step_degrees;   /**< Interpolation step (default: 2) */
    uint8_t smooth_delay_ms;       /**< Delay between steps (default: 15) */
} servo_config_t;

/**
 * @brief Initialize servo controller
 *
 * @param config Servo configuration
 * @return ESP_OK on success
 */
esp_err_t servo_init(const servo_config_t *config);

/**
 * @brief Get current servo angle
 *
 * @return Current angle in degrees
 */
uint8_t servo_get_angle(void);

/**
 * @brief Move servo to specific angle
 *
 * @param angle Target angle in degrees
 * @param smooth Use smooth interpolation if true
 * @return ESP_OK on success
 */
esp_err_t servo_move_to(uint8_t angle, bool smooth);

/**
 * @brief Step servo left by configured step angle
 *
 * Uses smooth interpolation for movement.
 *
 * @return ESP_OK on success
 */
esp_err_t servo_step_left(void);

/**
 * @brief Step servo right by configured step angle
 *
 * Uses smooth interpolation for movement.
 *
 * @return ESP_OK on success
 */
esp_err_t servo_step_right(void);

/**
 * @brief Center servo to default position
 *
 * @return ESP_OK on success
 */
esp_err_t servo_center(void);

/**
 * @brief Stop servo (no-op, servo holds position)
 */
void servo_stop(void);

/**
 * @brief Release servo (stop PWM signal)
 *
 * Servo becomes manually moveable after release.
 */
void servo_release(void);

/**
 * @brief Cleanup servo resources
 */
void servo_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* SERVO_CONTROL_H */
