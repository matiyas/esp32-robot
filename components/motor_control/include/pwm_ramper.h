/**
 * @file pwm_ramper.h
 * @brief PWM soft-start ramper for motor control
 *
 * Provides gradual PWM duty cycle ramping to prevent inrush
 * current spikes when starting motors.
 */

#ifndef PWM_RAMPER_H
#define PWM_RAMPER_H

#include <esp_err.h>

#include <stdbool.h>
#include <stdint.h>

#include "hal_pwm.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PWM ramper configuration
 */
typedef struct {
    uint32_t ramp_duration_ms; /**< Total ramp time (default: 500) */
    uint8_t num_steps;         /**< Number of duty cycle steps (default: 25) */
    uint8_t max_duty_percent;  /**< Maximum duty cycle 0-100 (default: 100) */
} pwm_ramper_config_t;

/**
 * @brief Initialize PWM ramper
 *
 * @param channel PWM channel to control
 * @param config Ramper configuration
 * @return ESP_OK on success
 */
esp_err_t pwm_ramper_init(hal_pwm_channel_t channel, const pwm_ramper_config_t *config);

/**
 * @brief Start ramp-up to full duty cycle
 *
 * Non-blocking operation that runs in FreeRTOS task.
 * Cancels any existing ramp before starting.
 */
void pwm_ramper_start(void);

/**
 * @brief Stop ramping and set duty to zero
 *
 * Immediately cancels any active ramp and stops PWM.
 */
void pwm_ramper_stop(void);

/**
 * @brief Set immediate duty cycle (bypass ramping)
 *
 * @param duty_percent Duty cycle 0-100
 */
void pwm_ramper_set_duty(uint8_t duty_percent);

/**
 * @brief Check if ramping is currently active
 *
 * @return true if ramp is in progress
 */
bool pwm_ramper_is_active(void);

/**
 * @brief Cleanup ramper resources
 */
void pwm_ramper_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* PWM_RAMPER_H */
