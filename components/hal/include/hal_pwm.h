/**
 * @file hal_pwm.h
 * @brief PWM Hardware Abstraction Layer
 *
 * Provides a platform-independent interface for PWM operations using
 * ESP32 LEDC peripheral. Supports both motor PWM and servo control.
 */

#ifndef HAL_PWM_H
#define HAL_PWM_H

#include "hal_types.h"
#include <esp_err.h>
#include <driver/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize PWM channel on GPIO pin
 *
 * Configures LEDC peripheral for PWM output on the specified pin.
 *
 * @param pin GPIO pin number for PWM output
 * @param frequency_hz PWM frequency in Hz
 * @param[out] channel Pointer to store allocated channel handle
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t hal_pwm_init(gpio_num_t pin, uint32_t frequency_hz, hal_pwm_channel_t *channel);

/**
 * @brief Set PWM duty cycle
 *
 * @param channel PWM channel handle
 * @param duty_percent Duty cycle 0-100
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t hal_pwm_set_duty(hal_pwm_channel_t channel, uint8_t duty_percent);

/**
 * @brief Set servo pulse width
 *
 * Configures PWM for servo control with specific pulse width.
 * Assumes 50Hz frequency for standard servos.
 *
 * @param channel PWM channel handle
 * @param pulse_us Pulse width in microseconds (typically 500-2400)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t hal_pwm_set_servo_pulse(hal_pwm_channel_t channel, uint16_t pulse_us);

/**
 * @brief Stop PWM output
 *
 * Sets duty cycle to 0 and stops the PWM signal.
 *
 * @param channel PWM channel handle
 */
void hal_pwm_stop(hal_pwm_channel_t channel);

/**
 * @brief Cleanup PWM channel
 *
 * Releases the PWM channel and associated resources.
 *
 * @param channel PWM channel handle
 */
void hal_pwm_cleanup(hal_pwm_channel_t channel);

/**
 * @brief Check if PWM channel is valid
 *
 * @param channel PWM channel handle
 * @return true if channel is valid, false otherwise
 */
bool hal_pwm_is_valid(hal_pwm_channel_t channel);

#ifdef __cplusplus
}
#endif

#endif /* HAL_PWM_H */
